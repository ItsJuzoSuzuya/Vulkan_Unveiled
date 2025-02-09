#include "render_system.hpp"
#include "buffer.hpp"
#include "device.hpp"
#include "frame_info.hpp"
#include "game_object.hpp"
#include "occlusion_culler.hpp"
#include "swapchain.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <glm/ext/scalar_constants.hpp>
#include <memory>
#include <ostream>
#include <unordered_map>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace engine {

struct PushConstantData {
  glm::mat4 modelMatrix{1.f};
  glm::mat4 normalMatrix{1.f};
};

RenderSystem::RenderSystem(Device &device, Window &window,
                           VkDescriptorSetLayout descriptorSetLayout)
    : device{device}, window{window} {
  swapChain = std::make_unique<SwapChain>(device, window.getExtent());
  createPipelineLayout(descriptorSetLayout);
  createPipeline(swapChain->getRenderPass());
  createCommandBuffers();
}

RenderSystem::~RenderSystem() {
  vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
  vkFreeCommandBuffers(device.device(), device.getCommandPool(), 1,
                       &imageAcquireCommandBuffer);
}

void RenderSystem::recreateSwapChain() {
  auto extent = window.getExtent();
  while (extent.width == 0 || extent.height == 0) {
    extent = window.getExtent();
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(device.device());

  std::shared_ptr<SwapChain> oldSwapChain = std::move(swapChain);
  swapChain = std::make_unique<SwapChain>(device, extent, oldSwapChain);

  if (!oldSwapChain->compareSwapFormats(*swapChain.get()))
    throw std::runtime_error("Swap chain image(or depth) format has changed!");
}

void RenderSystem::createPipelineLayout(
    VkDescriptorSetLayout descriptorSetLayout) {
  VkPushConstantRange pushConstantRange = {};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pushConstantRange.size = sizeof(PushConstantData);
  pushConstantRange.offset = 0;

  std::vector<VkDescriptorSetLayout> layouts{descriptorSetLayout};

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
  pipelineLayoutInfo.pSetLayouts = layouts.data();
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

  if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr,
                             &pipelineLayout) != VK_SUCCESS)
    throw std::runtime_error("Failed to create pipeline layout!");
}

void RenderSystem::createPipeline(VkRenderPass renderPass) {
  assert(pipelineLayout != nullptr &&
         "Cannot create pipeline before pipeline layout!");

  PipelineConfigInfo piplineConfigInfo = {};
  Pipeline::defaultPipelineConfig(piplineConfigInfo);
  piplineConfigInfo.renderPass = renderPass;
  piplineConfigInfo.pipelineLayout = pipelineLayout;

  pipeline = std::make_unique<Pipeline>(
      device, "src/shaders/shader.vert.spv", "src/shaders/terrain_control.spv",
      "src/shaders/terrain_evaluation.spv", "src/shaders/shader.frag.spv",
      piplineConfigInfo);
}

void RenderSystem::createCommandBuffers() {
  commandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = device.getCommandPool();
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

  if (vkAllocateCommandBuffers(device.device(), &allocInfo,
                               commandBuffers.data()) != VK_SUCCESS)
    throw std::runtime_error("Failed to allocate command buffers!");
}

VkCommandBuffer RenderSystem::beginFrame() {
  auto result = swapChain->acquireNextImage(&currentImageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return nullptr;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    throw std::runtime_error("Failed to acquire next swap chain image!");

  VkCommandBuffer commandBuffer = getCurrentCommandBuffer();
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    throw std::runtime_error("Failed to begin recording command buffer!");

  return commandBuffer;
}

void RenderSystem::recordCommandBuffer(VkCommandBuffer commandBuffer) {
  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = swapChain->getRenderPass();
  renderPassInfo.framebuffer = swapChain->getFrameBuffer(currentImageIndex);
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swapChain->extent();

  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};
  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport = {};
  viewport.x = 0;
  viewport.y = static_cast<float>(swapChain->extent().height);
  viewport.width = static_cast<float>(swapChain->extent().width);
  viewport.height = -static_cast<float>(swapChain->extent().height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = swapChain->extent();
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  pipeline->bind(commandBuffer);
}

void RenderSystem::renderWorld(FrameInfo &frameInfo, const Player &player,
                               std::unordered_map<int, Chunk> &chunks) {
  pipeline->bind(frameInfo.commandBuffer);

  vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                          &frameInfo.descriptorSet, 0, nullptr);

  glm::vec3 playerChunkPosition =
      glm::floor(player.transform.position / 32.f) * 32.f;

  auto it = chunks.begin();
  while (it != chunks.end()) {
    Chunk &chunk = it->second;

    if (chunk.blocks.size() == 1 && chunk.blocks[0] == BlockType::Air) {
      it++;
      continue;
    }

    if (frameInfo.camera.canSee(chunk.transform.position) ||
        chunk.transform.position == playerChunkPosition) {

      PushConstantData push{};
      push.modelMatrix = chunk.transform.mat4();
      push.normalMatrix = chunk.transform.normalMatrix();

      vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout,
                         VK_SHADER_STAGE_VERTEX_BIT, 0,
                         sizeof(PushConstantData), &push);

      chunk.model->bind(frameInfo.commandBuffer);
      chunk.model->draw(frameInfo.commandBuffer);
    }
    it++;
  }
}

void RenderSystem::getDepthBufferData(const FrameInfo &frameInfo,
                                      std::vector<float> &depthData,
                                      std::unique_ptr<Buffer> &stagingBuffer) {
  depthData.resize(swapChain->extent().width / 4 * swapChain->extent().height /
                   4);
  VkImage &depthImage = swapChain->getDepthImage(
      (frameInfo.frameIndex + SwapChain::MAX_FRAMES_IN_FLIGHT - 1) %
      SwapChain::MAX_FRAMES_IN_FLIGHT);

  stagingBuffer->getDepthBufferData(imageAcquireCommandBuffer, depthImage,
                                    swapChain->extent(), depthData);

  std::memcpy(depthData.data(), stagingBuffer->mappedData(),
              depthData.size() * sizeof(float));
}

void RenderSystem::renderGameObjects(FrameInfo &frameInfo,
                                     std::vector<GameObject> &gameObjects) {
  pipeline->bind(frameInfo.commandBuffer);

  vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                          &frameInfo.descriptorSet, 0, nullptr);

  for (GameObject &gameObject : gameObjects) {
    PushConstantData push{};
    push.modelMatrix = gameObject.transform.mat4();
    push.normalMatrix = gameObject.transform.normalMatrix();

    vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData),
                       &push);

    gameObject.model->bind(frameInfo.commandBuffer);
    gameObject.model->draw(frameInfo.commandBuffer);
  }
}

void RenderSystem::endRenderPass(VkCommandBuffer commandBuffer) {
  vkCmdEndRenderPass(commandBuffer);
}

void RenderSystem::endFrame() {
  VkCommandBuffer commandBuffer = getCurrentCommandBuffer();
  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    throw std::runtime_error("Failed to record command buffer!");

  auto result =
      swapChain->submitCommandBuffer(&commandBuffer, &currentImageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      window.wasWindowResized()) {
    window.resetWindowResizedFlag();
    recreateSwapChain();
    currentFrameIndex = 0;
    return;
  } else if (result != VK_SUCCESS)
    throw std::runtime_error("Failed to present swap chain image!");

  currentFrameIndex = (currentFrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
}
} // namespace engine

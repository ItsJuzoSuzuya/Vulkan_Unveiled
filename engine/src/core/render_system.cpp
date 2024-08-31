#include "render_system.hpp"
#include <cassert>
#include <iostream>
#include <memory>
#include <ostream>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace engine {

RenderSystem::RenderSystem(Device &device, VkRenderPass renderPass,
                           VkDescriptorSetLayout descriptorSetLayout)
    : device{device} {
  createPipelineLayout(descriptorSetLayout);
  std::cout << "Pipeline layout created\n";
  createPipeline(renderPass);
  std::cout << "Pipeline created\n";
}

RenderSystem::~RenderSystem() {
  vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
}

void RenderSystem::createPipelineLayout(
    VkDescriptorSetLayout descriptorSetLayout) {
  std::vector<VkDescriptorSetLayout> layouts{descriptorSetLayout};

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount =
      0; // static_cast<uint32_t>(layouts.size());
  pipelineLayoutInfo.pSetLayouts = nullptr; // layouts.data();

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

  std::cout << "Creating pipeline\n";

  pipeline = std::make_unique<Pipeline>(device, "src/shaders/shader.vert.spv",
                                        "src/shaders/shader.frag.spv",
                                        piplineConfigInfo);
}

} // namespace engine

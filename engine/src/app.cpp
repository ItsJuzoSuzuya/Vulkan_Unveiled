#include "app.hpp"
#include "collision.hpp"
#include "core/buffer.hpp"
#include "core/game_object.hpp"
#include "core/model.hpp"
#include "core/occlusion_culler.hpp"
#include "core/swapchain.hpp"
#include "include/stb_image.h"
#include "movement_controller.hpp"
#include "player.hpp"
#include <GLFW/glfw3.h>
#include <chrono>
#include <cstdint>
#include <glm/common.hpp>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define STB_IMAGE_IMPLEMENTATION

using namespace std;

namespace engine {

int RENDER_DISTANCE = 6;
const int MAX_DRAW_CALLS = 10000;
const int WIDTH = 1200;
const int HEIGHT = 800;

struct GlobalUbo {
  glm::mat4 projectionView{1.f};
  glm::vec4 ambientLightColor{1.f, 1.f, 1.f, .02f};
  glm::vec3 lightPosition{0.f, 20.f, 0.f};
  alignas(16) glm::vec4 lightColor{1.f};
};

App::App() {
  descriptorPool = DescriptorPool::Builder(device)
                       .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
                       .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                    SwapChain::MAX_FRAMES_IN_FLIGHT)
                       .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                    SwapChain::MAX_FRAMES_IN_FLIGHT)
                       .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                    SwapChain::MAX_FRAMES_IN_FLIGHT)
                       .build();
}

void App::run() {
  worldModel = make_unique<Model>(device);

  vector<shared_ptr<Buffer>> drawCallBuffers(SwapChain::MAX_FRAMES_IN_FLIGHT);
  for (int i = 0; i < static_cast<int>(drawCallBuffers.size()); i++) {
    drawCallBuffers[i] = make_unique<Buffer>(
        device, sizeof(VkDrawIndexedIndirectCommand), MAX_DRAW_CALLS,
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    drawCallBuffers[i]->map();
  }

  vector<shared_ptr<Buffer>> objectDataBuffers(SwapChain::MAX_FRAMES_IN_FLIGHT);
  for (int i = 0; i < static_cast<int>(objectDataBuffers.size()); i++) {
    objectDataBuffers[i] =
        make_unique<Buffer>(device, sizeof(ObjectData), MAX_DRAW_CALLS,
                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    objectDataBuffers[i]->map();
  }

  vector<unique_ptr<Buffer>> uboBuffers(SwapChain::MAX_FRAMES_IN_FLIGHT);
  for (int i = 0; i < static_cast<int>(uboBuffers.size()); i++) {
    uboBuffers[i] = make_unique<Buffer>(device, sizeof(GlobalUbo), 1,
                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    uboBuffers[i]->map();
  }

  int textureWidth, textureHeight, textureChannels;
  stbi_uc *textureData =
      stbi_load("textures/gras.jpeg", &textureWidth, &textureHeight,
                &textureChannels, STBI_rgb_alpha);
  VkDeviceSize textureSize = textureWidth * textureHeight * 4;

  if (!textureData) {
    cerr << "Failed to load texture image: " << stbi_failure_reason() << endl;
    throw runtime_error("Failed to load texture image");
  }

  Buffer textureStagingBuffer{device, textureSize, 1,
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
  textureStagingBuffer.map();
  textureStagingBuffer.writeToBuffer((void *)textureData);

  stbi_image_free(textureData);

  VkImage textureImage;
  VkDeviceMemory textureImageMemory;

  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = static_cast<uint32_t>(textureWidth);
  imageInfo.extent.height = static_cast<uint32_t>(textureHeight);
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.usage =
      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;

  device.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                             textureImage, textureImageMemory);
  device.transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                               VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  device.copyBufferToImage(textureStagingBuffer.getBuffer(), textureImage,
                           static_cast<uint32_t>(textureWidth),
                           static_cast<uint32_t>(textureHeight));
  device.transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  VkImageViewCreateInfo viewInfo = {};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = textureImage;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  VkImageView textureImageView;
  if (vkCreateImageView(device.device(), &viewInfo, nullptr,
                        &textureImageView) != VK_SUCCESS)
    throw std::runtime_error("Failed to create image views!");

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = 1.0f;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;

  VkSampler textureSampler;
  if (vkCreateSampler(device.device(), &samplerInfo, nullptr,
                      &textureSampler) != VK_SUCCESS)
    throw std::runtime_error("Failed to create texture sampler!");

  auto descriptorSetLayout =
      DescriptorSetLayout::Builder(device)
          .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                      VK_SHADER_STAGE_ALL_GRAPHICS)
          .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                      VK_SHADER_STAGE_VERTEX_BIT)
          .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                      VK_SHADER_STAGE_FRAGMENT_BIT)
          .build();

  vector<VkDescriptorSet> descriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT);
  for (int i = 0; i < static_cast<int>(descriptorSets.size()); i++) {
    auto bufferInfo = uboBuffers[i]->descriptorInfo();
    auto ODBInfo = objectDataBuffers[i]->descriptorInfo();
    auto imageInfo =
        VkDescriptorImageInfo{textureSampler, textureImageView,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    DescriptorWriter(*descriptorSetLayout, *descriptorPool)
        .writeBuffer(0, &bufferInfo)
        .writeBuffer(1, &ODBInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
        .writeImage(2, &imageInfo)
        .build(descriptorSets[i]);
  }

  RenderSystem renderSystem{device, window,
                            descriptorSetLayout->getDescriptorSetLayout()};

  ChunkLoader chunkLoader{device, chunkQueue, chunkUnloadQueue, queueMutex,
                          refreshChunks};

  Camera camera{};
  camera.setPerspectiveProjection(glm::radians(90.f),
                                  renderSystem.getAspectRatio(), 0.1f, 1000.f);

  MovementController movementController{};

  Player player = Player();
  player.transform.position = {7.f, 120.f, 8.f};
  player.transform.scale = {0.8f, 2.f, 0.8f};

  glm::vec3 colliderMin =
      player.transform.position - glm::vec3{player.transform.scale.x / 2.f, 0,
                                            player.transform.scale.z / 2.f};
  glm::vec3 colliderMax =
      player.transform.position + glm::vec3{player.transform.scale.x / 2.f,
                                            player.transform.scale.y,
                                            player.transform.scale.z / 2.f};
  player.collider = BoxCollider(player.transform.position, colliderMax);

  auto currentTime = chrono::high_resolution_clock::now();

  while (!window.shouldClose()) {
    /* GLFW Poll Events */

    auto *objectDataBuffer =
        (ObjectData *)objectDataBuffers[renderSystem.getFrameIndex()]
            ->mappedData();
    auto *drawCallBuffer = (VkDrawIndexedIndirectCommand *)
                               drawCallBuffers[renderSystem.getFrameIndex()]
                                   ->mappedData();

    glfwPollEvents();
    if (glfwGetKey(window.getGLFWwindow(), GLFW_KEY_ESCAPE)) {
      window.close();
      return;
    }

    /* Delta Time */

    auto newTime = chrono::high_resolution_clock::now();
    float deltaTime =
        chrono::duration<float, chrono::seconds::period>(newTime - currentTime)
            .count();
    currentTime = newTime;

    /* Movement */

    movementController.move(window.getGLFWwindow(), player, chunks, deltaTime);
    player.rigidBody.update(player.transform.position, deltaTime);
    camera.follow(player.transform.position, player.transform.rotation);

    /* Chunk Loading */

    chunkLoader.startChunkThread(player, chunks);
    chunkLoader.unloadOutOfRangeChunks(player, chunks);

    /* Gravity */

    if (chunks.size() > 100) {
      player.rigidBody.applyGravity(deltaTime);
      player.rigidBody.update(player.transform.position, deltaTime);
      player.collider.collisionBox.min =
          player.transform.position - glm::vec3{player.transform.scale.x / 2.f,
                                                0,
                                                player.transform.scale.z / 2.f};
      player.collider.collisionBox.max =
          player.transform.position + glm::vec3{player.transform.scale.x / 2.f,
                                                player.transform.scale.y,
                                                player.transform.scale.z / 2.f};
    }

    /* Collision Detection World */

    {
      lock_guard<mutex> lock(queueMutex);
      Block blockBeneathPlayer = player.getBlockBeneath(chunks);
      if (blockBeneathPlayer.type != BlockType::Air) {
        player.transform.position.y =
            blockBeneathPlayer.collider.collisionBox.max.y;
        player.canJump = true;
        player.rigidBody.resetVelocity();
      }
    }

    /* Rendering */

    while (!chunkQueue.empty()) {
      {
        lock_guard<mutex> lock(queueMutex);

        int chunkIndex =
            chunkLoader.getChunkIndex(chunkQueue.front().transform.position);

        chunks.insert({chunkIndex, std::move(chunkQueue.front())});
        chunkQueue.pop();
      }
    }

    if (auto commandBuffer = renderSystem.beginFrame()) {
      int frameIndex = renderSystem.getFrameIndex();

      loadWorldModel(objectDataBuffer, drawCallBuffer, player, camera,
                     frameIndex);
      GlobalUbo ubo{};
      ubo.projectionView = camera.getProjection() * camera.getView();
      uboBuffers[frameIndex]->writeToBuffer(&ubo);
      uboBuffers[frameIndex]->flush();

      FrameInfo frameInfo{frameIndex,
                          commandBuffer,
                          camera,
                          descriptorSets[frameIndex],
                          drawCallBuffers[frameIndex],
                          objectDataBuffers[frameIndex]};

      renderSystem.recordCommandBuffer(commandBuffer);
      renderSystem.renderWorld(frameInfo, worldModel, drawCallCounter);
      renderSystem.endRenderPass(commandBuffer);
      renderSystem.endFrame();
    }

    chrono::duration<double> frameTime =
        chrono::high_resolution_clock::now() - currentTime;
    // cout << "Frame Time: " << frameTime.count() * 1000 << "ms" << endl;
    //  cout << "\rFps: " << 1.0 / frameTime.count() << flush;
    frameCounter++;
  }
  chunkThread.join();
}

uint32_t App::loadWorldModel(ObjectData *objectDataBuffer,
                             VkDrawIndexedIndirectCommand *drawCallBuffer,
                             Player &player, Camera &camera,
                             uint32_t frameIndex) {
  glm::vec3 playerChunkPosition =
      glm::floor(player.transform.position / 32.f) * 32.f;

  drawCallCounter = 0;
  vertexBufferOffset = 0;
  indexBufferOffset = 0;

  int firstIndex = 0;

  uint32_t vertexOffset = 0;

  auto it = chunks.begin();

  while (it != chunks.end()) {

    Chunk &chunk = it->second;

    if (chunk.blocks.size() == 1 && chunk.blocks[0] == BlockType::Air) {
      it++;
      continue;
    }

    if (camera.canSee(chunk.transform.position) ||
        chunk.transform.position == playerChunkPosition) {

      objectDataBuffer[drawCallCounter] = {chunk.transform.mat4(),
                                           chunk.transform.normalMatrix()};

      VkDrawIndexedIndirectCommand indirectCommand{};
      indirectCommand.indexCount = chunk.getMesh().second.size();
      indirectCommand.instanceCount = 1;
      indirectCommand.firstIndex = firstIndex;
      indirectCommand.vertexOffset = vertexOffset;
      indirectCommand.firstInstance = drawCallCounter;
      drawCallBuffer[drawCallCounter] = indirectCommand;

      worldModel->writeMeshDataToBuffers(chunk.getMesh(), vertexBufferOffset,
                                         indexBufferOffset, frameIndex);

      drawCallCounter++;
      firstIndex += chunk.getMesh().second.size();
      vertexOffset += chunk.getMesh().first.size();
      vertexBufferOffset +=
          chunk.getMesh().first.size() * sizeof(Model::Vertex);
      indexBufferOffset += chunk.getMesh().second.size() * sizeof(uint32_t);
    }
    it++;
  }

  if (drawCallCounter == 0)
    return 0;

  device.copyModel(worldModel->stagingBuffers[frameIndex]->getBuffer(),
                   worldModel->vertexBuffers[frameIndex]->getBuffer(),
                   worldModel->indexBuffers[frameIndex]->getBuffer(),
                   vertexBufferOffset, indexBufferOffset);

  return drawCallCounter;
}

} // namespace engine

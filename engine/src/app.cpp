#include "app.hpp"
#include "collision.hpp"
#include "core/buffer.hpp"
#include "core/game_object.hpp"
#include "core/model.hpp"
#include "core/occlusion_culler.hpp"
#include "core/swapchain.hpp"
#include "movement_controller.hpp"
#include "player.hpp"
#include <GLFW/glfw3.h>
#include <chrono>
#include <cstdint>
#include <cstdlib>
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

  VkImageView textureImageView;
  VkSampler textureSampler;

  device.generateImage("textures/gras.jpeg", textureImageView, textureSampler);

  VkImageView topTextureImageView;
  VkSampler topTextureSampler;

  device.generateImage("textures/gras_top.jpg", topTextureImageView,
                       topTextureSampler);

  auto descriptorSetLayout =
      DescriptorSetLayout::Builder(device)
          .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                      VK_SHADER_STAGE_ALL_GRAPHICS)
          .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                      VK_SHADER_STAGE_VERTEX_BIT)
          .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                      VK_SHADER_STAGE_FRAGMENT_BIT)
          .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                      VK_SHADER_STAGE_FRAGMENT_BIT)
          .build();

  vector<VkDescriptorSet> descriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT);
  for (int i = 0; i < static_cast<int>(descriptorSets.size()); i++) {
    auto bufferInfo = uboBuffers[i]->descriptorInfo();
    auto ODBInfo = objectDataBuffers[i]->descriptorInfo();
    auto imageInfo =
        VkDescriptorImageInfo{textureSampler, textureImageView,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    auto topImageInfo =
        VkDescriptorImageInfo{topTextureSampler, topTextureImageView,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    DescriptorWriter(*descriptorSetLayout, *descriptorPool)
        .writeBuffer(0, &bufferInfo)
        .writeBuffer(1, &ODBInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
        .writeImage(2, &imageInfo)
        .writeImage(3, &topImageInfo)
        .build(descriptorSets[i]);
  }

  RenderSystem renderSystem{device, window,
                            descriptorSetLayout->getDescriptorSetLayout()};

  Camera camera{};
  camera.setPerspectiveProjection(glm::radians(90.f),
                                  renderSystem.getAspectRatio(), 0.1f, 1000.f);

  MovementController movementController{};

  Player player = Player();
  player.transform.position = {0.f, 80.f, 0.f};
  player.transform.scale = {0.8f, 2.f, 0.8f};

  ChunkLoader chunkLoader{device, chunkQueue, chunkUnloadQueue, queueMutex};
  chunkLoader.startChunkThread(player, chunks);

  glm::vec3 colliderMin =
      player.transform.position - glm::vec3{player.transform.scale.x / 2.f, 0,
                                            player.transform.scale.z / 2.f};
  glm::vec3 colliderMax =
      player.transform.position + glm::vec3{player.transform.scale.x / 2.f,
                                            player.transform.scale.y,
                                            player.transform.scale.z / 2.f};
  player.collider = BoxCollider(colliderMin, colliderMax);

  auto currentTime = chrono::high_resolution_clock::now();

  while (!window.shouldClose()) {
    /* GLFW Poll Events */

    glfwPollEvents();
    if (glfwGetKey(window.getGLFWwindow(), GLFW_KEY_ESCAPE))
      window.close();

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

    chunkLoader.unloadOutOfRangeChunks(player, chunks, freeChunks);

    /* Gravity */

    if (chunks.size() > 7) {
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

    queue<Chunk *> pushQueue;
    while (!chunkQueue.empty()) {
      {
        lock_guard<mutex> lock(queueMutex);

        int chunkIndex =
            chunkLoader.getChunkIndex(chunkQueue.front().transform.position);
        chunks.insert({chunkIndex, std::move(chunkQueue.front())});

        chunkQueue.pop();

        if (chunks.at(chunkIndex).blocks.size() != 1 ||
            chunks.at(chunkIndex).blocks[0].type != BlockType::Air)
          pushQueue.push(&chunks.at(chunkIndex));
      }
    }

    if (auto commandBuffer = renderSystem.beginFrame()) {
      int frameIndex = renderSystem.getFrameIndex();

      loadWorldModel(pushQueue, objectDataBuffers, drawCallBuffers);

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
    // cout << "\rFps: " << 1.0 / frameTime.count() << flush;
    frameCounter++;
  }

  chunkLoader.running = false;
  chunkThread.join();
}

void App::loadWorldModel(queue<Chunk *> &pushQueue,
                         vector<shared_ptr<Buffer>> objectDataBuffers,
                         vector<shared_ptr<Buffer>> drawCallBuffers) {
  if (pushQueue.empty())
    return;

  VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();
  while (!pushQueue.empty()) {
    Chunk &chunk = *pushQueue.front();

    if (freeChunks.empty()) {
      auto *objectDataBuffer = (ObjectData *)objectDataBuffers[0]->mappedData();
      auto *drawCallBuffer =
          (VkDrawIndexedIndirectCommand *)drawCallBuffers[0]->mappedData();
      objectDataBuffer[drawCallCounter] = {chunk.transform.mat4(),
                                           chunk.transform.normalMatrix()};

      VkDrawIndexedIndirectCommand indirectCommand{};
      indirectCommand.indexCount = chunk.getMesh().second.size();
      indirectCommand.instanceCount = 1;
      indirectCommand.firstIndex = firstIndex;
      indirectCommand.vertexOffset = vertexOffset;
      indirectCommand.firstInstance = drawCallCounter;
      drawCallBuffer[drawCallCounter] = indirectCommand;

      objectDataBuffer = (ObjectData *)objectDataBuffers[1]->mappedData();
      drawCallBuffer =
          (VkDrawIndexedIndirectCommand *)drawCallBuffers[1]->mappedData();
      objectDataBuffer[drawCallCounter] = {chunk.transform.mat4(),
                                           chunk.transform.normalMatrix()};

      drawCallBuffer[drawCallCounter] = indirectCommand;

      worldModel->writeMeshDataToBuffer(chunk.getMesh(), vertexBufferOffset,
                                        indexBufferOffset);

      BufferBlock bufferBlock;
      bufferBlock.drawCallIndex = drawCallCounter;
      bufferBlock.vertexOffset = vertexOffset;
      bufferBlock.vertexCount = chunk.getMesh().first.size();
      bufferBlock.firstIndex = firstIndex;
      bufferBlock.indexCount = chunk.getMesh().second.size();
      bufferBlock.vertexBufferOffset = vertexBufferOffset;
      bufferBlock.indexBufferOffset = indexBufferOffset;
      chunk.bufferMemory = bufferBlock;

      VkBufferCopy copyRegion = {};
      copyRegion.srcOffset = vertexBufferOffset;
      copyRegion.dstOffset = vertexBufferOffset;
      copyRegion.size = chunk.getMesh().first.size() * sizeof(Model::Vertex);
      vkCmdCopyBuffer(commandBuffer, worldModel->stagingBuffer->getBuffer(),
                      worldModel->ringBuffer->getBuffer(), 1, &copyRegion);

      copyRegion.srcOffset =
          indexBufferOffset + 10000000 * sizeof(Model::Vertex);
      copyRegion.dstOffset =
          indexBufferOffset + 10000000 * sizeof(Model::Vertex);
      copyRegion.size = chunk.getMesh().second.size() * sizeof(uint32_t);
      vkCmdCopyBuffer(commandBuffer, worldModel->stagingBuffer->getBuffer(),
                      worldModel->ringBuffer->getBuffer(), 1, &copyRegion);

      drawCallCounter++;
      firstIndex += 30000;
      vertexOffset += 20000;
      vertexBufferOffset += 20000 * sizeof(Model::Vertex);
      indexBufferOffset += 30000 * sizeof(uint32_t);
      pushQueue.pop();
    } else {
      auto freeChunk = freeChunks.front();

      auto *objectDataBuffer = (ObjectData *)objectDataBuffers[0]->mappedData();
      objectDataBuffer[freeChunk.drawCallIndex] = {
          chunk.transform.mat4(), chunk.transform.normalMatrix()};

      auto *drawCallBuffer =
          (VkDrawIndexedIndirectCommand *)drawCallBuffers[0]->mappedData();
      VkDrawIndexedIndirectCommand indirectCommand{};
      indirectCommand.indexCount = chunk.getMesh().second.size();
      indirectCommand.instanceCount = 1;
      indirectCommand.firstIndex = freeChunk.firstIndex;
      indirectCommand.vertexOffset = freeChunk.vertexOffset;
      indirectCommand.firstInstance = freeChunk.drawCallIndex;
      drawCallBuffer[freeChunk.drawCallIndex] = indirectCommand;

      objectDataBuffer = (ObjectData *)objectDataBuffers[1]->mappedData();
      drawCallBuffer =
          (VkDrawIndexedIndirectCommand *)drawCallBuffers[1]->mappedData();
      objectDataBuffer[freeChunk.drawCallIndex] = {
          chunk.transform.mat4(), chunk.transform.normalMatrix()};
      drawCallBuffer[freeChunk.drawCallIndex] = indirectCommand;

      worldModel->writeMeshDataToBuffer(chunk.getMesh(),
                                        freeChunk.vertexBufferOffset,
                                        freeChunk.indexBufferOffset);

      chunk.bufferMemory = freeChunk;

      VkBufferCopy copyRegion = {};
      copyRegion.srcOffset = copyRegion.dstOffset =
          freeChunk.vertexBufferOffset;
      copyRegion.size = chunk.getMesh().first.size() * sizeof(Model::Vertex);
      vkCmdCopyBuffer(commandBuffer, worldModel->stagingBuffer->getBuffer(),
                      worldModel->ringBuffer->getBuffer(), 1, &copyRegion);

      copyRegion.srcOffset = copyRegion.dstOffset =
          freeChunk.indexBufferOffset + 10000000 * sizeof(Model::Vertex);
      copyRegion.size = chunk.getMesh().second.size() * sizeof(uint32_t);
      vkCmdCopyBuffer(commandBuffer, worldModel->stagingBuffer->getBuffer(),
                      worldModel->ringBuffer->getBuffer(), 1, &copyRegion);

      pushQueue.pop();
      freeChunks.pop();
    }
  }
  device.endSingleTimeCommands(commandBuffer);
}

} // namespace engine

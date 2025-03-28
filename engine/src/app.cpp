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

int RENDER_DISTANCE = 5;
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
  player.collider = BoxCollider(colliderMin, colliderMax);

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

    size_t updatedChunks = 0;

    while (!chunkQueue.empty()) {
      {
        lock_guard<mutex> lock(queueMutex);

        int chunkIndex =
            chunkLoader.getChunkIndex(chunkQueue.front().transform.position);

        chunks.insert({chunkIndex, std::move(chunkQueue.front())});
        updatedChunks++;

        chunkQueue.pop();
      }
    }

    if (auto commandBuffer = renderSystem.beginFrame()) {
      int frameIndex = renderSystem.getFrameIndex();

      if (updatedChunks > 0)
        loadWorldModel(objectDataBuffer, drawCallBuffer, frameIndex);

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
    cout << "Frame Time: " << frameTime.count() * 1000 << "ms" << endl;
    // cout << "\rFps: " << 1.0 / frameTime.count() << flush;
    frameCounter++;
  }
  chunkThread.join();
}

uint32_t App::loadWorldModel(ObjectData *objectDataBuffer,
                             VkDrawIndexedIndirectCommand *drawCallBuffer,
                             uint32_t frameIndex) {
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
    vertexBufferOffset += chunk.getMesh().first.size() * sizeof(Model::Vertex);
    indexBufferOffset += chunk.getMesh().second.size() * sizeof(uint32_t);
    it++;
  }

  if (drawCallCounter == 0)
    return 0;

  size_t size = 10000000 * sizeof(Model::Vertex) + 15000000 * sizeof(uint32_t);

  device.copyBuffer(worldModel->stagingBuffer->getBuffer(),
                    worldModel->ringBuffer->getBuffer(), size,
                    frameIndex * size, frameIndex * size);

  return drawCallCounter;
}

} // namespace engine

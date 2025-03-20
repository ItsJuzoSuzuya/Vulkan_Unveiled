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
#include <ostream>
#include <thread>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

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

  auto descriptorSetLayout =
      DescriptorSetLayout::Builder(device)
          .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                      VK_SHADER_STAGE_ALL_GRAPHICS)
          .addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                      VK_SHADER_STAGE_VERTEX_BIT)
          .build();

  vector<VkDescriptorSet> descriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT);
  for (int i = 0; i < static_cast<int>(descriptorSets.size()); i++) {
    auto bufferInfo = uboBuffers[i]->descriptorInfo();
    auto ODBInfo = objectDataBuffers[i]->descriptorInfo();
    DescriptorWriter(*descriptorSetLayout, *descriptorPool)
        .writeBuffer(0, &bufferInfo)
        .writeBuffer(1, &ODBInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
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
  player.transform.position = {0.f, 120.f, 0.f};
  player.transform.scale = {0.8f, 2.f, 0.8f};

  glm::vec3 colliderMax = player.transform.position + player.transform.scale;

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

    movementController.move(window.getGLFWwindow(), deltaTime, player);
    camera.follow(player.transform.position + glm::vec3{0.f, 1.f, 0.f},
                  player.transform.rotation);

    /* Chunk Loading */

    chunkLoader.startChunkThread(player, chunks);
    chunkLoader.unloadOutOfRangeChunks(player, chunks);

    /* Gravity */

    if (chunks.size() > 100) {
      player.rigidBody.applyGravity(deltaTime);
      player.rigidBody.update(player.transform.position, deltaTime);
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

      vector<Block> blocksAroundPlayer = player.getBlocksAround(chunks);
      for (const auto &block : blocksAroundPlayer) {
        Collision3D collision =
            player.collider.resolveCollision(block.collider.collisionBox);
        if (collision.isColliding) {
          cout << "Colliding" << endl;
          player.transform.position += collision.direction * collision.length;
        }
      }
    }

    /* Rendering */

    auto startRender = chrono::high_resolution_clock::now();
    while (!chunkQueue.empty()) {
      {
        lock_guard<mutex> lock(queueMutex);

        int chunkIndex =
            chunkLoader.getChunkIndex(chunkQueue.front().transform.position);

        chunks.insert({chunkIndex, std::move(chunkQueue.front())});
        chunkQueue.pop();
      }
    }
    auto endRender = chrono::high_resolution_clock::now();

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
    cout << "Frame Time: " << frameTime.count() * 1000 << "ms" << endl;
    // cout << "\rFps: " << 1.0 / frameTime.count() << flush;
    frameCounter++;
  }
  vkDeviceWaitIdle(device.device());
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

  std::chrono::duration<double> meshDataTime =
      std::chrono::duration<double>::zero();

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
      auto endOcclusion = std::chrono::high_resolution_clock::now();

      auto startMeshData = std::chrono::high_resolution_clock::now();
      worldModel->writeMeshDataToBuffers(chunk.getMesh(), vertexBufferOffset,
                                         indexBufferOffset, frameIndex);
      auto endMeshData = std::chrono::high_resolution_clock::now();
      meshDataTime += endMeshData - startMeshData;

      drawCallCounter++;
      firstIndex += chunk.getMesh().second.size();
      vertexOffset += chunk.getMesh().first.size();
      vertexBufferOffset +=
          chunk.getMesh().first.size() * sizeof(Model::Vertex);
      indexBufferOffset += chunk.getMesh().second.size() * sizeof(uint32_t);
    }
    it++;
  }

  device.copyModel(worldModel->stagingBuffers[frameIndex]->getBuffer(),
                   worldModel->vertexBuffers[frameIndex]->getBuffer(),
                   worldModel->indexBuffers[frameIndex]->getBuffer(),
                   vertexBufferOffset, indexBufferOffset);

  return drawCallCounter;
}

} // namespace engine

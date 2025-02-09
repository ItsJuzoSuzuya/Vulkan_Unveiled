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

using namespace std;

namespace engine {

int RENDER_DISTANCE = 8;
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

  loadGameObjects();
}

void App::run() {
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
    DescriptorWriter(*descriptorSetLayout, *descriptorPool)
        .writeBuffer(0, &bufferInfo)
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
  player.transform.position = {0.f, 100.f, 0.f};
  player.transform.scale = {0.8f, 2.f, 0.8f};

  auto currentTime = chrono::high_resolution_clock::now();

  VkDeviceSize instanceSize = sizeof(float);
  uint32_t instanceCount = int(WIDTH / 4) * int(HEIGHT / 4);

  int frameCount = 0;
  while (!window.shouldClose()) {
    /* GLFW Poll Events */

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
      player.rigidBody.applyGravity();
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
          player.transform.position += collision.direction * collision.length;
        }
      }
    }

    /* Rendering */

    while (!chunkQueue.empty()) {
      {
        lock_guard<mutex> lock(queueMutex);

        if (chunkQueue.front().blocks.size() != 1)
          chunkQueue.front().model =
              make_shared<Model>(device, chunkQueue.front().getMesh().first,
                                 chunkQueue.front().getMesh().second);

        int chunkIndex =
            chunkLoader.getChunkIndex(chunkQueue.front().transform.position);

        chunks.insert({chunkIndex, std::move(chunkQueue.front())});
        chunkQueue.pop();
      }
    }

    if (auto commandBuffer = renderSystem.beginFrame()) {
      int frameIndex = renderSystem.getFrameIndex();

      GlobalUbo ubo{};
      ubo.projectionView = camera.getProjection() * camera.getView();
      uboBuffers[frameIndex]->writeToBuffer(&ubo);
      uboBuffers[frameIndex]->flush();

      FrameInfo frameInfo{frameIndex, commandBuffer, camera,
                          descriptorSets[frameIndex]};

      renderSystem.recordCommandBuffer(commandBuffer);
      renderSystem.renderWorld(frameInfo, player, chunks);
      renderSystem.renderGameObjects(frameInfo, gameObjects);
      renderSystem.endRenderPass(commandBuffer);
      renderSystem.endFrame();
    }

    chrono::duration<double> frameTime =
        chrono::high_resolution_clock::now() - currentTime;
    cout << "\rFps: " << 1.0 / frameTime.count() << flush;
  }
  vkDeviceWaitIdle(device.device());
  chunkThread.join();
}

void App::loadGameObjects() {
  shared_ptr<Model> cubeModel =
      Model::createModelFromFile(device, "models/cube/scene.gltf");

  GameObject floor = GameObject::create();
  floor.model = cubeModel;
  floor.transform.position = {0.f, 0.f, 0.f};
  floor.transform.scale = {1.f, 1.f, 100.f};
}

} // namespace engine

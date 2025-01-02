#include "app.hpp"
#include "core/buffer.hpp"
#include "core/game_object.hpp"
#include "core/model.hpp"
#include "core/swapchain.hpp"
#include "movement_controller.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

namespace engine {

const int RENDER_DISTANCE = 8;

struct GlobalUbo {
  glm::mat4 projectionView{1.f};
  glm::vec4 ambientLightColor{1.f, 1.f, 1.f, .02f};
  glm::vec3 lightPosition{100.f, 20.f, 100.f};
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
  std::vector<std::unique_ptr<Buffer>> uboBuffers(
      SwapChain::MAX_FRAMES_IN_FLIGHT);
  for (int i = 0; i < uboBuffers.size(); i++) {
    uboBuffers[i] = std::make_unique<Buffer>(
        device, sizeof(GlobalUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
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

  std::vector<VkDescriptorSet> descriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT);
  for (int i = 0; i < descriptorSets.size(); i++) {
    auto bufferInfo = uboBuffers[i]->descriptorInfo();
    DescriptorWriter(*descriptorSetLayout, *descriptorPool)
        .writeBuffer(0, &bufferInfo)
        .build(descriptorSets[i]);
  }

  RenderSystem renderSystem{device, window,
                            descriptorSetLayout->getDescriptorSetLayout()};

  Camera camera{};
  camera.setPerspectiveProjection(
      glm::radians(90.f), renderSystem.getAspectRatio(), 0.1f, 1000000.f);

  MovementController movementController{};

  GameObject player = GameObject::createGameObject();
  player.transform.position = {0.f, 20.f, 0.f};

  auto currentTime = std::chrono::high_resolution_clock::now();

  ChunkLoader chunkLoader{device, chunkQueue, queueMutex, refreshChunks};

  int frameCount = 0;

  while (!window.shouldClose()) {
    glfwPollEvents();

    if (glfwGetKey(window.getGLFWwindow(), GLFW_KEY_ESCAPE))
      window.close();

    auto newTime = std::chrono::high_resolution_clock::now();
    float deltaTime =
        std::chrono::duration<float, std::chrono::seconds::period>(newTime -
                                                                   currentTime)
            .count();
    currentTime = newTime;

    movementController.move(window.getGLFWwindow(), deltaTime, player);
    player.transform.rotation.x =
        std::clamp(player.transform.rotation.x, -glm::half_pi<float>(),
                   glm::half_pi<float>());
    camera.follow(player.transform.position, player.transform.rotation);

    checkChunks(player.transform.position, player.transform.rotation);

    if (refreshChunks) {
      chunkLoader.startChunkThread(player);
      refreshChunks = false;
    }

    {
      std::lock_guard<std::mutex> lock(queueMutex);
      while (!chunkQueue.empty()) {
        GameObject chunk = GameObject::createGameObject();
        chunk.transform.position = chunkQueue.front().getPosition();
        chunk.model =
            std::make_shared<Model>(device, chunkQueue.front().getMesh().first,
                                    chunkQueue.front().getMesh().second);

        gameObjects.push_back(std::move(chunk));
        chunkQueue.pop();
      }
    }

    if (auto commandBuffer = renderSystem.beginFrame()) {
      int frameIndex = renderSystem.getFrameIndex();
      FrameInfo frameInfo{frameIndex, commandBuffer, camera,
                          descriptorSets[frameIndex]};

      GlobalUbo ubo{};
      ubo.projectionView = camera.getProjection() * camera.getView();
      uboBuffers[frameIndex]->writeToBuffer(&ubo);
      uboBuffers[frameIndex]->flush();

      renderSystem.recordCommandBuffer(commandBuffer);
      renderSystem.renderGameObjects(frameInfo, gameObjects);
      renderSystem.endRenderPass(commandBuffer);
      renderSystem.endFrame();
      frameCount++;
    }
  }
  vkDeviceWaitIdle(device.device());
  chunkThread.join();
}

void App::loadGameObjects() {
  std::shared_ptr<Model> cubeModel =
      Model::createModelFromFile(device, "models/cube/scene.gltf");

  GameObject floor = GameObject::createGameObject();
  floor.model = cubeModel;
  floor.transform.position = {0.f, -20.f, 0.f};
}

void App::checkChunks(const glm::vec3 &playerChunk,
                      const glm::vec3 &playerRotation) {
  for (int i = 0; i < gameObjects.size(); i++) {
    glm::vec3 chunkDirection = gameObjects[i].transform.position - playerChunk;
    float chunkDistance =
        glm::length(glm::vec2{chunkDirection.x, chunkDirection.z});

    if (chunkDistance > 8 * 32) {
      vkDeviceWaitIdle(device.device());
      gameObjects.erase(gameObjects.begin() + i);
    }
  }
}

} // namespace engine

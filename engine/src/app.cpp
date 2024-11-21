#include "app.hpp"
#include "core/buffer.hpp"
#include "core/model.hpp"
#include "core/swapchain.hpp"
#include "movement_controller.hpp"
#include <chrono>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/trigonometric.hpp>
#include <memory>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

namespace engine {

struct GlobalUbo {
  glm::mat4 projectionView{1.f};
};

App::App() {
  descriptorPool = DescriptorPool::Builder(device)
                       .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
                       .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
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
  camera.setPerspectiveProjection(glm::radians(90.f),
                                  renderSystem.getAspectRatio(), 0.1f, 10.f);

  MovementController movementController{};

  GameObject player = GameObject::createGameObject();

  auto currentTime = std::chrono::high_resolution_clock::now();

  while (!window.shouldClose()) {
    glfwPollEvents();

    auto newTime = std::chrono::high_resolution_clock::now();
    float deltaTime =
        std::chrono::duration<float, std::chrono::seconds::period>(newTime -
                                                                   currentTime)
            .count();
    currentTime = newTime;

    movementController.move(window.getGLFWwindow(), deltaTime, player);
    camera.follow(player.transform.position, player.transform.rotation);

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
    }
  }
  vkDeviceWaitIdle(device.device());
}

void App::loadGameObjects() {
  std::shared_ptr<Model> cubeModel =
      Model::createModelFromFile(device, "models/cube/scene.gltf");

  GameObject cube = GameObject::createGameObject();
  cube.model = cubeModel;
  cube.transform.position = {0.f, 0.f, 2.f};
  cube.transform.scale = {0.5f, 0.5f, 0.5f};
  cube.transform.rotation = {0.f, glm::radians(30.f), glm::radians(45.f)};

  gameObjects.push_back(std::move(cube));
}
} // namespace engine

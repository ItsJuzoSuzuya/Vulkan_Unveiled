#include "app.hpp"
#include "core/model.hpp"
#include "core/render_system.hpp"
#include <glm/trigonometric.hpp>
#include <memory>
#include <utility>
#include <vulkan/vulkan_core.h>

namespace engine {
App::App() { loadGameObjects(); }

void App::run() {
  RenderSystem renderSystem{device, window, nullptr};

  while (!window.shouldClose()) {
    glfwPollEvents();
    drawFrame(renderSystem);
  }
  vkDeviceWaitIdle(device.device());
}

void App::drawFrame(RenderSystem &renderSystem) {
  if (auto commandBuffer = renderSystem.beginFrame()) {
    renderSystem.recordCommandBuffer(commandBuffer);
    renderSystem.renderGameObjects(commandBuffer, gameObjects);
    renderSystem.endRenderPass(commandBuffer);
    renderSystem.endFrame();
  }
}

void App::loadGameObjects() {
  std::shared_ptr<Model> cubeModel =
      Model::createModelFromFile(device, "models/cube/scene.gltf");
  GameObject cube = GameObject::createGameObject();
  cube.model = cubeModel;
  cube.transform.position = {0.f, 0.f, 1.f};
  cube.transform.scale = {0.5f, 0.5f, 0.5f};
  cube.transform.rotation = {0.f, 0.f, glm::radians(45.f)};

  gameObjects.push_back(std::move(cube));
}
} // namespace engine

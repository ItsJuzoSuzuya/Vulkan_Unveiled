#include "app.hpp"
#include "core/render_system.hpp"
#include <memory>
#include <vulkan/vulkan_core.h>

namespace engine {
App::App() {
  Model::Builder builder = {};
  builder.loadModel("models/cube/scene.gltf");
  model = std::make_shared<Model>(device, builder);
}

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
    renderSystem.recordCommandBuffer(commandBuffer, model);
    renderSystem.endFrame();
  }
}
} // namespace engine

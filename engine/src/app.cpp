#include "app.hpp"
#include "core/render_system.hpp"
#include <vulkan/vulkan_core.h>

namespace engine {
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
    renderSystem.endFrame();
  }
}
} // namespace engine

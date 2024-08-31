#include "app.hpp"
#include "core/render_system.hpp"

namespace engine {
void App::run() {
  swapChain = std::make_unique<SwapChain>(device, window.getExtent());
  RenderSystem renderSystem{device, swapChain->getRenderPass(), nullptr};
  while (!window.shouldClose()) {
    glfwPollEvents();
  }
}
} // namespace engine

#include "app.hpp"

namespace engine {
void App::run() {
  while (!window.shouldClose()) {
    glfwPollEvents();
  }
}
} // namespace engine

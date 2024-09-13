#ifndef APP_HPP
#define APP_HPP
#include "core/render_system.hpp"

namespace engine {
class App {
public:
  static constexpr int WIDTH = 800;
  static constexpr int HEIGHT = 600;

  void run();

private:
  Window window{WIDTH, HEIGHT, "Vulkan Application"};
  Device device{window};

  void drawFrame(RenderSystem &renderSystem);
};
} // namespace engine
#endif

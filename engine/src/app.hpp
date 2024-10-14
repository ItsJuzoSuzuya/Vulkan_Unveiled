#ifndef APP_HPP
#define APP_HPP
#include "core/render_system.hpp"
#include <memory>

namespace engine {
class App {
public:
  static constexpr int WIDTH = 800;
  static constexpr int HEIGHT = 600;

  App();

  void run();

private:
  Window window{WIDTH, HEIGHT, "Vulkan Application"};
  Device device{window};

  void drawFrame(RenderSystem &renderSystem);

  std::shared_ptr<Model> model;
};
} // namespace engine
#endif

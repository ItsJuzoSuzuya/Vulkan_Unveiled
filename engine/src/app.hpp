#ifndef APP_HPP
#define APP_HPP
#include "core/descriptors.hpp"
#include "core/render_system.hpp"
#include <vector>

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

  std::unique_ptr<DescriptorPool> descriptorPool;

  void loadGameObjects();

  std::vector<GameObject> gameObjects;
};
} // namespace engine
#endif

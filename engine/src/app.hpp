#include "core/device.hpp"

namespace engine {
class App {
public:
  static constexpr int WIDTH = 800;
  static constexpr int HEIGHT = 600;

  void run();

private:
  Window window{WIDTH, HEIGHT, "Vulkan Application"};
  Device device{window};
};
} // namespace engine

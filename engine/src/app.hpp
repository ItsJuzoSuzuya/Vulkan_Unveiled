#ifndef APP_HPP
#define APP_HPP
#include "core/swapchain.hpp"
#include <memory>

namespace engine {
class App {
public:
  static constexpr int WIDTH = 800;
  static constexpr int HEIGHT = 600;

  void run();

private:
  Window window{WIDTH, HEIGHT, "Vulkan Application"};
  Device device{window};

  std::unique_ptr<SwapChain> swapChain;
};
} // namespace engine
#endif

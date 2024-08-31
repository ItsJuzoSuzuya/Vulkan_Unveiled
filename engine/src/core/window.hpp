#ifndef WINDOW_HPP
#define WINDOW_HPP
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace engine {

class Window {
public:
  Window(int width, int height, std::string name);
  ~Window();

  bool shouldClose() { return glfwWindowShouldClose(window); }

  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;

  VkExtent2D getExtent() {
    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
  }

  void createSurface(VkInstance instance, VkSurfaceKHR *surface) {
    if (glfwCreateWindowSurface(instance, window, nullptr, surface) !=
        VK_SUCCESS)
      throw std::runtime_error("Surface Creation was unsuccessful");
  }

private:
  void initWindow();

  const int width;
  const int height;

  std::string name;
  GLFWwindow *window;
};
} // namespace engine
#endif

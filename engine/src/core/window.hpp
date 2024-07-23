#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace vulkan_unveiled {

class Window {
public:
  Window(int width, int height, std::string name);
  ~Window();

private:
  void initWindow();

  const int width;
  const int height;

  std::string name;
  GLFWwindow *window;
};
} // namespace vulkan_unveiled

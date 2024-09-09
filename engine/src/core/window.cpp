#include "window.hpp"
#include <GLFW/glfw3.h>

namespace engine {

Window::Window(int width, int height, std::string name)
    : width(width), height(height), name(name) {
  initWindow();
}

Window::~Window() {
  glfwDestroyWindow(window);
  glfwTerminate();
}

void Window::initWindow() {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
}

} // namespace engine

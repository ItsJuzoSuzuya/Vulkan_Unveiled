#include "movement_controller.hpp"
#include <GLFW/glfw3.h>

namespace engine {

void MovementController::move(GLFWwindow *window, float dt,
                              GameObject &gameObject) {
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    gameObject.transform.position.x -= 1.0f * dt;
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    gameObject.transform.position.x += 1.0f * dt;
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    gameObject.transform.position.z += 1.0f * dt;
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    gameObject.transform.position.z -= 1.0f * dt;
};

} // namespace engine

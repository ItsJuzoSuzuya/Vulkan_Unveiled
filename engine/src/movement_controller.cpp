#include "movement_controller.hpp"
#include <glm/ext/vector_float3.hpp>

namespace engine {

void MovementController::move(GLFWwindow *window, float dt,
                              GameObject &gameObject) {
  if (firstMouse) {
    glfwGetCursorPos(window, &mouseX, &mouseY);
    firstMouse = false;
    return;
  }

  double newMouseX, newMouseY;
  glfwGetCursorPos(window, &newMouseX, &newMouseY);

  double deltaX = newMouseX - mouseX;
  double deltaY = newMouseY - mouseY;

  glm::vec3 rotateDirection{0.f};
  rotateDirection.y = deltaX;
  rotateDirection.x = deltaY;

  gameObject.transform.rotation += rotateDirection * dt;

  float yaw = gameObject.transform.rotation.y;

  const glm::vec3 forwardDirection{sin(yaw), 0, cos(yaw)};
  const glm::vec3 rightDirection{forwardDirection.z, 0, -forwardDirection.x};

  glm::vec3 moveDirection{0.f};
  if (glfwGetKey(window, keys.left) == GLFW_PRESS)
    moveDirection = -rightDirection;
  if (glfwGetKey(window, keys.right) == GLFW_PRESS)
    moveDirection = rightDirection;
  if (glfwGetKey(window, keys.forward) == GLFW_PRESS)
    moveDirection = forwardDirection;
  if (glfwGetKey(window, keys.backward) == GLFW_PRESS)
    moveDirection = -forwardDirection;

  gameObject.transform.position += moveDirection * dt;

  mouseX = newMouseX;
  mouseY = newMouseY;
};

} // namespace engine

#include "movement_controller.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <chrono>
#include <glm/ext/vector_float3.hpp>

namespace engine {

void MovementController::move(GLFWwindow *window, float dt, Player &player) {
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

  player.transform.rotation += rotateDirection * 0.05f;
  player.transform.rotation.x =
      std::clamp(player.transform.rotation.x, -glm::half_pi<float>(),
                 glm::half_pi<float>());

  float yaw = player.transform.rotation.y;

  const glm::vec3 forwardDirection{sin(yaw), 0, cos(yaw)};
  const glm::vec3 rightDirection{forwardDirection.z, 0, -forwardDirection.x};

  glm::vec3 moveDirection{0.f};
  if (glfwGetKey(window, keys.left) == GLFW_PRESS)
    moveDirection -= rightDirection;
  if (glfwGetKey(window, keys.right) == GLFW_PRESS)
    moveDirection += rightDirection;
  if (glfwGetKey(window, keys.forward) == GLFW_PRESS)
    moveDirection += forwardDirection;
  if (glfwGetKey(window, keys.backward) == GLFW_PRESS)
    moveDirection -= forwardDirection;
  if (glfwGetKey(window, keys.up) == GLFW_PRESS) {
    if (player.canJump) {
      player.rigidBody.applyForce({0.f, 10.f, 0.f});
      player.canJump = false;
    }
  }

  player.transform.position += moveDirection * dt * 10.f;
  player.collider.collisionBox.min += moveDirection * dt * 10.f;
  player.collider.collisionBox.max += moveDirection * dt * 10.f;

  mouseX = newMouseX;
  mouseY = newMouseY;
};

} // namespace engine

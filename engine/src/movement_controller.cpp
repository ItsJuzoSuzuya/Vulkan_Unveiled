#include "movement_controller.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <glm/ext/vector_float3.hpp>
#include <unordered_map>

using namespace std;
namespace engine {

void MovementController::move(GLFWwindow *window, Player &player,
                              unordered_map<int, Chunk> &chunks, float dt) {
  if (firstMouse) {
    glfwGetCursorPos(window, &mouseX, &mouseY);
    firstMouse = false;
    return;
  }

  handleInput(window, player, dt);
  predictMovement(player, dt);

  auto blocks = player.getBlocksAround(chunks);

  for (auto &block : blocks) {
    bool isColliding =
        player.collider.checkCollision(block.collider.collisionBox);
    if (isColliding) {
      auto collision =
          player.collider.resolveCollision(block.collider.collisionBox);
      if (collision.direction.x != 0.f) {
        player.rigidBody.velocity.x = 0.f;
      }
      if (collision.direction.z != 0.f) {
        player.rigidBody.velocity.z = 0.f;
      }
    }
  }
};

void MovementController::handleInput(GLFWwindow *window, Player &player,
                                     float dt) {
  double newMouseX, newMouseY;
  glfwGetCursorPos(window, &newMouseX, &newMouseY);

  double deltaX = newMouseX - mouseX;
  double deltaY = newMouseY - mouseY;

  glm::vec3 rotateDirection{0.f};
  rotateDirection.y = deltaX * dt;
  rotateDirection.x = deltaY * dt;

  player.transform.rotation += rotateDirection;
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

  player.rigidBody.velocity.x = moveDirection.x * 10.f;
  player.rigidBody.velocity.z = moveDirection.z * 10.f;

  mouseX = newMouseX;
  mouseY = newMouseY;
}

void MovementController::predictMovement(Player &player, float dt) {
  player.collider.collisionBox.min =
      player.collider.collisionBox.min + player.rigidBody.velocity * dt;
  player.collider.collisionBox.max =
      player.collider.collisionBox.max + player.rigidBody.velocity * dt;
}

} // namespace engine

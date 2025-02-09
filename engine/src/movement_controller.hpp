#include "core/game_object.hpp"
#include "player.hpp"
#include <GLFW/glfw3.h>

namespace engine {

class MovementController {
public:
  struct KeyMapping {
    int forward = GLFW_KEY_W;
    int backward = GLFW_KEY_S;
    int left = GLFW_KEY_A;
    int right = GLFW_KEY_D;
    int up = GLFW_KEY_SPACE;
    int down = GLFW_KEY_LEFT_SHIFT;
  };

  void move(GLFWwindow *window, float dt, Player &player);

private:
  KeyMapping keys{};
  double mouseX, mouseY;
  bool firstMouse = true;
};

} // namespace engine

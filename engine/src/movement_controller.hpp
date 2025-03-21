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

  void move(GLFWwindow *window, Player &player,
            std::unordered_map<int, Chunk> &chunks, float dt);

private:
  KeyMapping keys{};
  double mouseX, mouseY;
  bool firstMouse = true;

  void handleInput(GLFWwindow *window, Player &player, float dt);
  void predictMovement(Player &player, float dt);
};

} // namespace engine

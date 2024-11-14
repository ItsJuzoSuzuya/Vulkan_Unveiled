#include "core/game_object.hpp"
#include <GLFW/glfw3.h>

namespace engine {

class MovementController {
public:
  void move(GLFWwindow *window, float dt, GameObject &gameObject);
};

} // namespace engine

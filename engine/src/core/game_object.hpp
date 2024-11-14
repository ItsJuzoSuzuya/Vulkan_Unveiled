#pragma once
#include "model.hpp"
#include <glm/ext/matrix_transform.hpp>

namespace engine {
struct Transform {
  glm::vec3 position{};
  glm::vec3 rotation{};
  glm::vec3 scale{1.f, 1.f, 1.f};

  glm::mat4 mat4();
};

class GameObject {
public:
  using id_t = unsigned int;

  static GameObject createGameObject() {
    static id_t currentId = 0;
    return GameObject{currentId++};
  }

  GameObject(const GameObject &) = delete;
  GameObject &operator=(const GameObject &) = delete;
  GameObject(GameObject &&) = default;
  GameObject &operator=(GameObject &&) = default;

  id_t getId() { return id; }

  std::shared_ptr<Model> model{};
  Transform transform{};

private:
  id_t id;
  GameObject(id_t id) : id{id} {}
};
} // namespace engine

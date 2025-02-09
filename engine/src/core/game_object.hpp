#pragma once
#include "model.hpp"
#include <glm/ext/matrix_transform.hpp>
#include <typeindex>
#include <unordered_map>

namespace engine {

struct Component {};

struct Transform : Component {
  Transform() = default;
  Transform(glm::vec3 position) : position{position} {}
  Transform(float x, float y, float z) : Transform(glm::vec3(x, y, z)) {}

  glm::vec3 position{};
  glm::vec3 rotation{};
  glm::vec3 scale{1.f, 1.f, 1.f};

  glm::mat4 mat4() const;
  glm::mat4 normalMatrix() const;
};

class GameObject {
public:
  using id_t = unsigned int;

  GameObject() {}
  GameObject(const GameObject &) = delete;
  GameObject &operator=(const GameObject &) = delete;
  GameObject(GameObject &&) = default;
  GameObject &operator=(GameObject &&) = default;

  static GameObject create() {
    static id_t currentId = 0;
    return GameObject{currentId++};
  }

  id_t getId() { return id; }

  std::unordered_map<std::type_index, Component> components;

  std::shared_ptr<Model> model{};
  Transform transform{};

private:
  id_t id;
  GameObject(id_t id) : id{id} {}
};
} // namespace engine

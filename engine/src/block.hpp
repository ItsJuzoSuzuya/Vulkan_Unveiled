#pragma once
#include "collision.hpp"
#include "core/game_object.hpp"
#include <cstdint>
#include <sys/types.h>

namespace engine {
struct BlockType {
  const static uint8_t Air = 0;
  const static uint8_t Stone = 1;

  uint8_t type;

  operator int() const { return type; }
  BlockType &operator=(const int &other) {
    type = other;
    return *this;
  }
};

class Block : public GameObject {
public:
  Block(glm::vec3 position) : GameObject() {
    this->transform = Transform{position};
    collider.collisionBox.min = position;
    collider.collisionBox.max = position + glm::vec3(1.0f);
  }
  Block(Transform transform) : GameObject() {
    this->transform = transform;
    collider = BoxCollider{transform};
  }

  BlockType type;
  BoxCollider collider;
};

} // namespace engine

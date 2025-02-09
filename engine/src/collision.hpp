#pragma once
#include "core/game_object.hpp"
#include <glm/glm.hpp>

namespace engine {
struct CollisionBox3D {
  glm::vec3 min;
  glm::vec3 max;
  glm::vec3 center() const { return (min + max) / 2.0f; }
  std::vector<Transform> corners() const {
    std::vector<Transform> corners;
    corners.push_back(Transform(min));
    corners.push_back(Transform(min.x, min.y, max.z));
    corners.push_back(Transform(min.x, max.y, min.z));
    corners.push_back(Transform(min.x, max.y, max.z));
    corners.push_back(Transform(max.x, min.y, min.z));
    corners.push_back(Transform(max.x, min.y, max.z));
    corners.push_back(Transform(max.x, max.y, min.z));
    corners.push_back(Transform(max));
    return corners;
  }
};

class Collision3D {
public:
  bool isColliding;

  glm::vec3 direction;
  float length;
};

class BoxCollider {
public:
  BoxCollider() { BoxCollider(Transform()); }
  BoxCollider(const Transform &transform);
  BoxCollider(const glm::vec3 &min, const glm::vec3 &max);

  bool checkCollision(const CollisionBox3D &other);
  Collision3D resolveCollision(const CollisionBox3D &other);

  Transform transform;
  CollisionBox3D collisionBox;
};

} // namespace engine

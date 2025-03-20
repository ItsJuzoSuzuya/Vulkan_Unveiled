#include "collision.hpp"
#include "core/game_object.hpp"
#include <algorithm>
#include <glm/fwd.hpp>

using namespace std;

namespace engine {

BoxCollider::BoxCollider(const Transform &transform) : transform{transform} {
  collisionBox.min = transform.position - transform.scale;
  collisionBox.max = transform.position + transform.scale;
}

BoxCollider::BoxCollider(const glm::vec3 &min, const glm::vec3 &max) {
  collisionBox.min = min;
  collisionBox.max = max;
  transform.position = min;
  transform.scale = max - min;
}

bool BoxCollider::checkCollision(const CollisionBox3D &other) {
  return (collisionBox.min.x <= other.max.x &&
          collisionBox.max.x >= other.min.x) &&
         (collisionBox.min.y <= other.max.y &&
          collisionBox.max.y >= other.min.y) &&
         (collisionBox.min.z <= other.max.z &&
          collisionBox.max.z >= other.min.z);
}

Collision3D BoxCollider::resolveCollision(const CollisionBox3D &other) {
  Collision3D collision;
  collision.isColliding = false;

  cout << "other: " << other.min.x << " " << other.min.y << " " << other.min.z
       << " " << other.max.x << " " << other.max.y << " " << other.max.z
       << endl; // Debug
  cout << "this: " << collisionBox.min.x << " " << collisionBox.min.y << " "
       << collisionBox.min.z << " " << collisionBox.max.x << " "
       << collisionBox.max.y << " " << collisionBox.max.z << endl; // Debug

  if (checkCollision(other)) {
    cout << "Collision detected" << endl;
    collision.isColliding = true;

    float xOverlap = std::min(collisionBox.max.x, other.max.x) -
                     std::max(collisionBox.min.x, other.min.x);
    float yOverlap = std::min(collisionBox.max.y, other.max.y) -
                     std::max(collisionBox.min.y, other.min.y);
    float zOverlap = std::min(collisionBox.max.z, other.max.z) -
                     std::max(collisionBox.min.z, other.min.z);

    if (xOverlap < yOverlap && xOverlap < zOverlap) {
      collision.direction = {1, 0, 0};
      collision.length = xOverlap;
    } else if (yOverlap < xOverlap && yOverlap < zOverlap) {
      collision.direction = {0, 1, 0};
      collision.length = yOverlap;
    } else {
      collision.direction = {0, 0, 1};
      collision.length = zOverlap;
    }

    if (collisionBox.min.x < other.min.x)
      collision.direction.x *= -1;
    if (collisionBox.min.y < other.min.y)
      collision.direction.y *= -1;
    if (collisionBox.min.z < other.min.z)
      collision.direction.z *= -1;
  }
  return collision;
}

} // namespace engine

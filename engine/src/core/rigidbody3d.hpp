#include <glm/ext/vector_float3.hpp>
namespace engine {

class RigidBody3D {
public:
  float gravity = 1.f;

  glm::vec3 velocity{0.f};

  void applyGravity();
  void applyForce(const glm::vec3 &force);
  void update(glm::vec3 &position, const float deltaTime);
  void resetVelocity();
};
} // namespace engine

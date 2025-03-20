#include <glm/ext/vector_float3.hpp>
namespace engine {

class RigidBody3D {
public:
  float gravity = 10.f;

  glm::vec3 velocity{0.f};

  void applyGravity(const float deltaTime);
  void applyForce(const glm::vec3 &force);
  void update(glm::vec3 &position, const float deltaTime);
  void resetVelocity();
};
} // namespace engine

#include "rigidbody3d.hpp"

namespace engine {

void RigidBody3D::applyGravity(const float dt) { velocity.y -= gravity * dt; }

void RigidBody3D::applyForce(const glm::vec3 &force) { velocity += force; }

void RigidBody3D::update(glm::vec3 &position, const float deltaTime) {
  position += velocity * deltaTime;
}

void RigidBody3D::resetVelocity() { velocity = glm::vec3{0.f}; }

} // namespace engine

#pragma once
#include "chunk.hpp"
#include "collision.hpp"
#include "core/rigidbody3d.hpp"
#include <glm/fwd.hpp>
#include <unordered_map>
#include <vector>

namespace engine {

class Player : public GameObject {
public:
  Block getBlockBeneath(const std::unordered_map<int, Chunk> &chunks);
  std::vector<Block>
  getBlocksAround(const std::unordered_map<int, Chunk> &chunks);

  bool canJump = false;

  BoxCollider collider{};
  RigidBody3D rigidBody{};
};

} // namespace engine

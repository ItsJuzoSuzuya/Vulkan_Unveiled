#include "player.hpp"
#include "app.hpp"
#include "block.hpp"
#include <glm/common.hpp>
#include <glm/fwd.hpp>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace engine {

Block Player::getBlockBeneath(const std::unordered_map<int, Chunk> &chunks) {
  glm::vec3 playerBlockPosition = glm::floor(transform.position);
  glm::vec3 playerChunkPosition = glm::floor(transform.position / 32.f) * 32.f;

  int playerBlockX = ((int)playerBlockPosition.x % 32 + 32) % 32;
  int playerBlockY = ((int)playerBlockPosition.y % 32 + 32) % 32;
  int playerBlockZ = ((int)playerBlockPosition.z % 32 + 32) % 32;

  Block blockBeneath{playerBlockPosition};

  bool isPlayerAtBottomOfChunk = int(playerBlockPosition.y) % 32 == 0;

  if (isPlayerAtBottomOfChunk) {
    try {
      const Chunk &chunk = chunks.at(
          playerChunkPosition.x + playerChunkPosition.z * RENDER_DISTANCE * 2 +
          (playerChunkPosition.y - 32) * RENDER_DISTANCE * RENDER_DISTANCE * 4);
      blockBeneath.type = chunk.getBlock(playerBlockX, 31, playerBlockZ);
      return blockBeneath;
    } catch (std::out_of_range) {
      return Block{Transform(0, 0, 0)};
    }
  } else {
    try {
      const Chunk &chunk = chunks.at(
          playerChunkPosition.x + playerChunkPosition.z * RENDER_DISTANCE * 2 +
          playerChunkPosition.y * RENDER_DISTANCE * RENDER_DISTANCE * 4);

      blockBeneath.type =
          chunk.getBlock(playerBlockX, playerBlockY - 1, playerBlockZ);

      return blockBeneath;
    } catch (std::out_of_range) {
      return Block{Transform(0, 0, 0)};
    }
  }

  return Block{Transform(0, 0, 0)};
}

std::vector<Block>
Player::getBlocksAround(const std::unordered_map<int, Chunk> &chunks) {
  glm::vec3 playerChunkPosition = glm::floor(transform.position / 32.f) * 32.f;

  glm::vec3 playerBlockPosition = glm::floor(transform.position);

  int playerBlockX = ((int)playerBlockPosition.x % 32 + 32) % 32;
  int playerBlockY = ((int)playerBlockPosition.y % 32 + 32) % 32;
  int playerBlockZ = ((int)playerBlockPosition.z % 32 + 32) % 32;

  bool isPlayerAtFrontEdge = playerBlockZ == 31;
  bool isPlayerAtBackEdge = playerBlockZ == 0;
  bool isPlayerAtLeftEdge = playerBlockX == 0;
  bool isPlayerAtRightEdge = playerBlockX == 31;

  Block frontBlock{playerBlockPosition + glm::vec3{0, 0, 1}};
  Block frontBlockAbove{playerBlockPosition + glm::vec3{0, 1, 1}};
  Block backBlock{playerBlockPosition + glm::vec3{0, 0, -1}};
  Block backBlockAbove{playerBlockPosition + glm::vec3{0, 1, -1}};

  Block leftBlock{playerBlockPosition + glm::vec3{-1, 0, 0}};
  Block leftBlockAbove{playerBlockPosition + glm::vec3{-1, 1, 0}};
  Block frontLeftBlock{playerBlockPosition + glm::vec3{-1, 0, 1}};
  Block frontLeftBlockAbove{playerBlockPosition + glm::vec3{-1, 1, 1}};

  Block rightBlock{playerBlockPosition + glm::vec3{1, 0, 0}};
  Block rightBlockAbove{playerBlockPosition + glm::vec3{1, 1, 0}};
  Block frontRightBlock{playerBlockPosition + glm::vec3{1, 0, 1}};
  Block frontRightBlockAbove{playerBlockPosition + glm::vec3{1, 1, 1}};

  Block backLeftBlock{playerBlockPosition + glm::vec3{-1, 0, -1}};
  Block backLeftBlockAbove{playerBlockPosition + glm::vec3{-1, 1, -1}};
  Block backRightBlock{playerBlockPosition + glm::vec3{1, 0, -1}};
  Block backRightBlockAbove{playerBlockPosition + glm::vec3{1, 1, -1}};

  const Chunk *chunkFront = nullptr;
  const Chunk *chunkBack = nullptr;
  const Chunk *chunkLeft = nullptr;
  const Chunk *chunkRight = nullptr;
  const Chunk *chunkFrontLeft = nullptr;
  const Chunk *chunkFrontRight = nullptr;
  const Chunk *chunkBackLeft = nullptr;
  const Chunk *chunkBackRight = nullptr;
  const Chunk *chunkPlayer = nullptr;

  std::vector<Block> surroundingBlocks;

  try {
    if (isPlayerAtFrontEdge)
      chunkFront = &chunks.at(
          playerChunkPosition.x +
          (playerChunkPosition.z + 32) * RENDER_DISTANCE * 2 +
          playerChunkPosition.y * RENDER_DISTANCE * 2 * RENDER_DISTANCE * 2);
    if (isPlayerAtBackEdge)
      chunkBack = &chunks.at(
          playerChunkPosition.x +
          (playerChunkPosition.z - 32) * RENDER_DISTANCE * 2 +
          playerChunkPosition.y * RENDER_DISTANCE * 2 * RENDER_DISTANCE * 2);
    if (isPlayerAtLeftEdge)
      chunkLeft = &chunks.at((playerChunkPosition.x - 32) +
                             playerChunkPosition.z * RENDER_DISTANCE * 2 +
                             playerChunkPosition.y * RENDER_DISTANCE * 2 *
                                 RENDER_DISTANCE * 2);
    if (isPlayerAtRightEdge)
      chunkRight = &chunks.at((playerChunkPosition.x + 32) +
                              playerChunkPosition.z * RENDER_DISTANCE * 2 +
                              playerChunkPosition.y * RENDER_DISTANCE * 2 *
                                  RENDER_DISTANCE * 2);

    if (isPlayerAtFrontEdge && isPlayerAtLeftEdge)
      chunkFrontLeft = &chunks.at(
          (playerChunkPosition.x - 32) +
          (playerChunkPosition.z + 32) * RENDER_DISTANCE * 2 +
          playerChunkPosition.y * RENDER_DISTANCE * 2 * RENDER_DISTANCE * 2);
    if (isPlayerAtFrontEdge && isPlayerAtRightEdge)
      chunkFrontRight = &chunks.at(
          (playerChunkPosition.x + 32) +
          (playerChunkPosition.z + 32) * RENDER_DISTANCE * 2 +
          playerChunkPosition.y * RENDER_DISTANCE * 2 * RENDER_DISTANCE * 2);
    if (isPlayerAtBackEdge && isPlayerAtLeftEdge)
      chunkBackLeft = &chunks.at(
          (playerChunkPosition.x - 32) +
          (playerChunkPosition.z - 32) * RENDER_DISTANCE * 2 +
          playerChunkPosition.y * RENDER_DISTANCE * 2 * RENDER_DISTANCE * 2);
    if (isPlayerAtBackEdge && isPlayerAtRightEdge)
      chunkBackRight = &chunks.at(
          (playerChunkPosition.x + 32) +
          (playerChunkPosition.z - 32) * RENDER_DISTANCE * 2 +
          playerChunkPosition.y * RENDER_DISTANCE * 2 * RENDER_DISTANCE * 2);

    chunkPlayer = &chunks.at(
        playerChunkPosition.x + playerChunkPosition.z * RENDER_DISTANCE * 2 +
        playerChunkPosition.y * RENDER_DISTANCE * 2 * RENDER_DISTANCE * 2);
  } catch (std::out_of_range) {
    return surroundingBlocks;
  }

  // Block behind player
  if (chunkBack) {
    backBlock.type = chunkBack->getBlock(playerBlockX, playerBlockY - 1, 31);
    backBlockAbove.type = chunkBack->getBlock(playerBlockX, playerBlockY, 31);
  } else {
    backBlock.type =
        chunkPlayer->getBlock(playerBlockX, playerBlockY - 1, playerBlockZ - 1);
    backBlockAbove.type =
        chunkPlayer->getBlock(playerBlockX, playerBlockY, playerBlockZ - 1);
  }

  if (backBlock.type != BlockType::Air)
    surroundingBlocks.push_back(std::move(backBlock));
  if (backBlockAbove.type != BlockType::Air)
    surroundingBlocks.push_back(std::move(backBlockAbove));

  // Block in front of player
  if (chunkFront) {
    frontBlock.type = chunkFront->getBlock(playerBlockX, playerBlockY - 1, 0);
    frontBlockAbove.type = chunkFront->getBlock(playerBlockX, playerBlockY, 0);
  } else {
    frontBlock.type =
        chunkPlayer->getBlock(playerBlockX, playerBlockY - 1, playerBlockZ + 1);
    frontBlockAbove.type =
        chunkPlayer->getBlock(playerBlockX, playerBlockY, playerBlockZ + 1);
  }

  if (frontBlock.type != BlockType::Air)
    surroundingBlocks.push_back(std::move(frontBlock));
  if (frontBlockAbove.type != BlockType::Air)
    surroundingBlocks.push_back(std::move(frontBlockAbove));

  // Block to the left of player
  if (chunkLeft) {
    leftBlock.type = chunkLeft->getBlock(31, playerBlockY - 1, playerBlockZ);
    leftBlockAbove.type = chunkLeft->getBlock(31, playerBlockY, playerBlockZ);
  } else {
    leftBlock.type =
        chunkPlayer->getBlock(playerBlockX - 1, playerBlockY - 1, playerBlockZ);
    leftBlockAbove.type =
        chunkPlayer->getBlock(playerBlockX - 1, playerBlockY, playerBlockZ);
  }

  if (leftBlock.type != BlockType::Air)
    surroundingBlocks.push_back(std::move(leftBlock));
  if (leftBlockAbove.type != BlockType::Air)
    surroundingBlocks.push_back(std::move(leftBlockAbove));

  // Block to the right of player
  if (chunkRight) {
    rightBlock.type = chunkRight->getBlock(0, playerBlockY - 1, playerBlockZ);
    rightBlockAbove.type = chunkRight->getBlock(0, playerBlockY, playerBlockZ);
  } else {
    rightBlock.type =
        chunkPlayer->getBlock(playerBlockX + 1, playerBlockY - 1, playerBlockZ);
    rightBlockAbove.type =
        chunkPlayer->getBlock(playerBlockX + 1, playerBlockY, playerBlockZ);
  }

  if (rightBlock.type != BlockType::Air)
    surroundingBlocks.push_back(std::move(rightBlock));
  if (rightBlockAbove.type != BlockType::Air)
    surroundingBlocks.push_back(std::move(rightBlockAbove));

  // Block to the back left of player
  if (chunkBackLeft) {
    backLeftBlock.type = chunkBackLeft->getBlock(31, playerBlockY - 1, 31);
    backLeftBlockAbove.type = chunkBackLeft->getBlock(31, playerBlockY, 31);
  } else if (chunkBack) {
    backLeftBlock.type =
        chunkBack->getBlock(playerBlockX - 1, playerBlockY - 1, 31);
    backLeftBlockAbove.type =
        chunkBack->getBlock(playerBlockX - 1, playerBlockY, 31);
  } else if (chunkLeft) {
    backLeftBlock.type =
        chunkLeft->getBlock(31, playerBlockY - 1, playerBlockZ - 1);
    backLeftBlockAbove.type =
        chunkLeft->getBlock(31, playerBlockY, playerBlockZ - 1);
  } else {
    backLeftBlock.type = chunkPlayer->getBlock(
        playerBlockX - 1, playerBlockY - 1, playerBlockZ - 1);
    backLeftBlockAbove.type =
        chunkPlayer->getBlock(playerBlockX - 1, playerBlockY, playerBlockZ - 1);
  }

  if (backLeftBlock.type != BlockType::Air)
    surroundingBlocks.push_back(std::move(backLeftBlock));
  if (backLeftBlockAbove.type != BlockType::Air)
    surroundingBlocks.push_back(std::move(backLeftBlockAbove));

  // Block to the back right of player
  if (chunkBackRight) {
    backRightBlock.type = chunkBackRight->getBlock(0, playerBlockY - 1, 31);
    backRightBlockAbove.type = chunkBackRight->getBlock(0, playerBlockY, 31);
  } else if (chunkBack) {
    backRightBlock.type =
        chunkBack->getBlock(playerBlockX + 1, playerBlockY - 1, 31);
    backRightBlockAbove.type =
        chunkBack->getBlock(playerBlockX + 1, playerBlockY, 31);
  } else if (chunkRight) {
    backRightBlock.type =
        chunkRight->getBlock(0, playerBlockY - 1, playerBlockZ - 1);
    backRightBlockAbove.type =
        chunkRight->getBlock(0, playerBlockY, playerBlockZ - 1);
  } else {
    backRightBlock.type = chunkPlayer->getBlock(
        playerBlockX + 1, playerBlockY - 1, playerBlockZ - 1);
    backRightBlockAbove.type =
        chunkPlayer->getBlock(playerBlockX + 1, playerBlockY, playerBlockZ - 1);
  }

  if (backRightBlock.type != BlockType::Air)
    surroundingBlocks.push_back(std::move(backRightBlock));
  if (backRightBlockAbove.type != BlockType::Air)
    surroundingBlocks.push_back(std::move(backRightBlockAbove));

  // Block to the front left of player
  if (chunkFrontLeft) {
    frontLeftBlock.type = chunkFrontLeft->getBlock(31, playerBlockY - 1, 0);
    frontLeftBlockAbove.type = chunkFrontLeft->getBlock(31, playerBlockY, 0);
  } else if (chunkFront) {
    frontLeftBlock.type =
        chunkFront->getBlock(playerBlockX - 1, playerBlockY - 1, 0);
    frontLeftBlockAbove.type =
        chunkFront->getBlock(playerBlockX - 1, playerBlockY, 0);
  } else if (chunkLeft) {
    frontLeftBlock.type =
        chunkLeft->getBlock(31, playerBlockY - 1, playerBlockZ + 1);
    frontLeftBlockAbove.type =
        chunkLeft->getBlock(31, playerBlockY, playerBlockZ + 1);
  } else {
    frontLeftBlock.type = chunkPlayer->getBlock(
        playerBlockX - 1, playerBlockY - 1, playerBlockZ + 1);
    frontLeftBlockAbove.type =
        chunkPlayer->getBlock(playerBlockX - 1, playerBlockY, playerBlockZ + 1);
  }

  if (frontLeftBlock.type != BlockType::Air)
    surroundingBlocks.push_back(std::move(frontLeftBlock));
  if (frontLeftBlockAbove.type != BlockType::Air)
    surroundingBlocks.push_back(std::move(frontLeftBlockAbove));

  // Block to the front right of player
  if (chunkFrontRight) {
    frontRightBlock.type = chunkFrontRight->getBlock(0, playerBlockY - 1, 0);
    frontRightBlockAbove.type = chunkFrontRight->getBlock(0, playerBlockY, 0);
  } else if (chunkFront) {
    frontRightBlock.type =
        chunkFront->getBlock(playerBlockX + 1, playerBlockY - 1, 0);
    frontRightBlockAbove.type =
        chunkFront->getBlock(playerBlockX + 1, playerBlockY, 0);
  } else if (chunkRight) {
    frontRightBlock.type =
        chunkRight->getBlock(0, playerBlockY - 1, playerBlockZ + 1);
    frontRightBlockAbove.type =
        chunkRight->getBlock(0, playerBlockY, playerBlockZ + 1);
  } else {
    frontRightBlock.type = chunkPlayer->getBlock(
        playerBlockX + 1, playerBlockY - 1, playerBlockZ + 1);
    frontRightBlockAbove.type =
        chunkPlayer->getBlock(playerBlockX + 1, playerBlockY, playerBlockZ + 1);
  }

  if (frontRightBlock.type != BlockType::Air)
    surroundingBlocks.push_back(std::move(frontRightBlock));
  if (frontRightBlockAbove.type != BlockType::Air)
    surroundingBlocks.push_back(std::move(frontRightBlockAbove));

  return surroundingBlocks;
}

} // namespace engine

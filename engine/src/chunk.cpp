#include "chunk.hpp"
#include "core/device.hpp"
#include "core/model.hpp"
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
namespace engine {
Chunk::Chunk(Device &device, std::vector<BlockType> blocks, glm::vec3 &position)
    : device{device}, blocks{blocks}, position{position} {
  calculateMesh();
};

void Chunk::calculateMesh() {
  std::vector<Model::Vertex> vertices;
  std::vector<uint32_t> indices;

  for (int y = 0; y < 32; y++) {
    for (int z = 0; z < 32; z++) {
      for (int x = 0; x < 32; x++) {
        BlockType block = blocks[x + z * 32 + y * 32 * 32];
        if (block.id == 0)
          continue;

        BlockType top =
            y < 31 ? blocks[x + z * 32 + (y + 1) * 32 * 32] : BlockType{0};
        BlockType bottom =
            y > 0 ? blocks[x + z * 32 + (y - 1) * 32 * 32] : BlockType{0};
        BlockType front =
            x > 0 ? blocks[(x - 1) + z * 32 + y * 32 * 32] : BlockType{0};
        BlockType back =
            x < 31 ? blocks[(x + 1) + z * 32 + y * 32 * 32] : BlockType{0};
        BlockType left =
            z > 0 ? blocks[x + (z - 1) * 32 + y * 32 * 32] : BlockType{0};
        BlockType right =
            z < 31 ? blocks[x + (z + 1) * 32 + y * 32 * 32] : BlockType{0};

        // Top
        if (y < 31) {
          if (top.id == 0) {
            vertices.push_back(
                {{x, y + 1, z}, {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}});
            vertices.push_back(
                {{x + 1, y + 1, z}, {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}});
            vertices.push_back(
                {{x + 1, y + 1, z + 1}, {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}});
            vertices.push_back(
                {{x, y + 1, z + 1}, {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}});

            indices.push_back(vertices.size() - 4);
            indices.push_back(vertices.size() - 3);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 4);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 1);
          }
        } else {
          vertices.push_back({{x, y + 1, z}, {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}});
          vertices.push_back(
              {{x + 1, y + 1, z}, {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}});
          vertices.push_back(
              {{x + 1, y + 1, z + 1}, {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}});
          vertices.push_back(
              {{x, y + 1, z + 1}, {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}});

          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 3);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 1);
        }

        // Bottom
        if (y > 0) {
          if (bottom.id == 0) {
            vertices.push_back({{x, y, z}, {0.f, 1.f, 0.f}, {0.f, -1.f, 0.f}});
            vertices.push_back(
                {{x + 1, y, z}, {0.f, 1.f, 0.f}, {0.f, -1.f, 0.f}});
            vertices.push_back(
                {{x + 1, y, z + 1}, {0.f, 1.f, 0.f}, {0.f, -1.f, 0.f}});
            vertices.push_back(
                {{x, y, z + 1}, {0.f, 1.f, 0.f}, {0.f, -1.f, 0.f}});
            indices.push_back(vertices.size() - 4);
            indices.push_back(vertices.size() - 3);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 4);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 1);
          }
        } else {
          vertices.push_back({{x, y, z}, {0.f, 1.f, 0.f}, {0.f, -1.f, 0.f}});
          vertices.push_back(
              {{x + 1, y, z}, {0.f, 1.f, 0.f}, {0.f, -1.f, 0.f}});
          vertices.push_back(
              {{x + 1, y, z + 1}, {0.f, 1.f, 0.f}, {0.f, -1.f, 0.f}});
          vertices.push_back(
              {{x, y, z + 1}, {0.f, 1.f, 0.f}, {0.f, -1.f, 0.f}});
          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 3);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 1);
        }

        // Front
        if (x > 0) {
          if (front.id == 0) {
            vertices.push_back({{x, y, z}, {0.f, 0.f, 1.f}, {-1.f, 0.f, 0.f}});
            vertices.push_back(
                {{x, y + 1, z}, {0.f, 0.f, 1.f}, {-1.f, 0.f, 0.f}});
            vertices.push_back(
                {{x, y + 1, z + 1}, {0.f, 0.f, 1.f}, {-1.f, 0.f, 0.f}});
            vertices.push_back(
                {{x, y, z + 1}, {0.f, 0.f, 1.f}, {-1.f, 0.f, 0.f}});
            indices.push_back(vertices.size() - 4);
            indices.push_back(vertices.size() - 3);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 4);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 1);
          }
        } else {
          vertices.push_back({{x, y, z}, {0.f, 0.f, 1.f}, {-1.f, 0.f, 0.f}});
          vertices.push_back(
              {{x, y + 1, z}, {0.f, 0.f, 1.f}, {-1.f, 0.f, 0.f}});
          vertices.push_back(
              {{x, y + 1, z + 1}, {0.f, 0.f, 1.f}, {-1.f, 0.f, 0.f}});
          vertices.push_back(
              {{x, y, z + 1}, {0.f, 0.f, 1.f}, {-1.f, 0.f, 0.f}});
          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 3);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 1);
        }

        // Back
        if (x < 31) {
          if (back.id == 0) {
            vertices.push_back(
                {{x + 1, y, z}, {0.f, 1.f, 1.f}, {1.f, 0.f, 0.f}});
            vertices.push_back(
                {{x + 1, y + 1, z}, {0.f, 1.f, 1.f}, {1.f, 0.f, 0.f}});
            vertices.push_back(
                {{x + 1, y + 1, z + 1}, {0.f, 1.f, 1.f}, {1.f, 0.f, 0.f}});
            vertices.push_back(
                {{x + 1, y, z + 1}, {0.f, 1.f, 1.f}, {1.f, 0.f, 0.f}});
            indices.push_back(vertices.size() - 4);
            indices.push_back(vertices.size() - 3);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 4);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 1);
          }
        } else {
          vertices.push_back({{x + 1, y, z}, {0.f, 1.f, 1.f}, {1.f, 0.f, 0.f}});
          vertices.push_back(
              {{x + 1, y + 1, z}, {0.f, 1.f, 1.f}, {1.f, 0.f, 0.f}});
          vertices.push_back(
              {{x + 1, y + 1, z + 1}, {0.f, 1.f, 1.f}, {1.f, 0.f, 0.f}});
          vertices.push_back(
              {{x + 1, y, z + 1}, {0.f, 1.f, 1.f}, {1.f, 0.f, 0.f}});
          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 3);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 1);
        }

        // Left
        if (z > 0) {
          if (left.id == 0) {
            vertices.push_back({{x, y, z}, {1.f, 0.f, 1.f}, {0.f, 0.f, -1.f}});
            vertices.push_back(
                {{x, y + 1, z}, {1.f, 0.f, 1.f}, {0.f, 0.f, -1.f}});
            vertices.push_back(
                {{x + 1, y + 1, z}, {1.f, 0.f, 1.f}, {0.f, 0.f, -1.f}});
            vertices.push_back(
                {{x + 1, y, z}, {1.f, 0.f, 1.f}, {0.f, 0.f, -1.f}});
            indices.push_back(vertices.size() - 4);
            indices.push_back(vertices.size() - 3);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 4);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 1);
          }
        } else {
          vertices.push_back({{x, y, z}, {1.f, 0.f, 1.f}, {0.f, 0.f, -1.f}});
          vertices.push_back(
              {{x, y + 1, z}, {1.f, 0.f, 1.f}, {0.f, 0.f, -1.f}});
          vertices.push_back(
              {{x + 1, y + 1, z}, {1.f, 0.f, 1.f}, {0.f, 0.f, -1.f}});
          vertices.push_back(
              {{x + 1, y, z}, {1.f, 0.f, 1.f}, {0.f, 0.f, -1.f}});
          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 3);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 1);
        }

        // Right
        if (z < 31) {
          if (right.id == 0) {
            vertices.push_back(
                {{x, y, z + 1}, {1.f, 1.f, 0.f}, {0.f, 0.f, 1.f}});
            vertices.push_back(
                {{x, y + 1, z + 1}, {1.f, 1.f, 0.f}, {0.f, 0.f, 1.f}});
            vertices.push_back(
                {{x + 1, y + 1, z + 1}, {1.f, 1.f, 0.f}, {0.f, 0.f, 1.f}});
            vertices.push_back(
                {{x + 1, y, z + 1}, {1.f, 1.f, 0.f}, {0.f, 0.f, 1.f}});
            indices.push_back(vertices.size() - 4);
            indices.push_back(vertices.size() - 3);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 4);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 1);
          }
        } else {
          vertices.push_back({{x, y, z + 1}, {1.f, 1.f, 0.f}, {0.f, 0.f, 1.f}});
          vertices.push_back(
              {{x, y + 1, z + 1}, {1.f, 1.f, 0.f}, {0.f, 0.f, 1.f}});
          vertices.push_back(
              {{x + 1, y + 1, z + 1}, {1.f, 1.f, 0.f}, {0.f, 0.f, 1.f}});
          vertices.push_back(
              {{x + 1, y, z + 1}, {1.f, 1.f, 0.f}, {0.f, 0.f, 1.f}});
          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 3);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 1);
        }
      }
    }
  }

  chunkMesh = {vertices, indices};
};

Chunk ChunkGenerator::generate(glm::vec3 position) {
  std::vector<BlockType> blocks;
  for (int y = 0; y < 32; y++) {
    for (int z = 0; z < 32; z++) {
      for (int x = 0; x < 32; x++) {
        BlockType block;
        block.id = getBlockType(position.x + x, position.y + y, position.z + z);
        blocks.push_back(block);
      }
    }
  }
  Chunk chunk{device, blocks, position};

  return chunk;
};

int ChunkGenerator::getBlockType(int x, int y, int z) {
  BlockType block;
  int surfaceY = 0;
  surfaceY = surfaceY + HeightMap::perlinNoise(x * 0.5, z * 0.5) * 32;

  if (y < surfaceY)
    return 1;
  else
    return 0;
}

bool ChunkLoader::isChunkLoaded(const glm::vec3 &chunkPosition,
                                std::vector<GameObject> &loadedChunks) {
  for (GameObject &chunk : loadedChunks) {
    if (chunk.transform.position.x == chunkPosition.x &&
        chunk.transform.position.z == chunkPosition.z) {
      return true;
    }
  }
  return false;
}

void ChunkLoader::startChunkThread(GameObject &player) {
  if (chunkThread.joinable()) {
    chunkThread.join();
  }

  chunkThread = std::thread(&ChunkLoader::loadChunks, this, std::ref(player));
}

void ChunkLoader::loadChunks(GameObject &player) {
  glm::vec3 playerChunk{player.transform.position / 32.f};

  for (int i = 0; i < 256; i++) {
    glm::vec3 playerViewDirection{glm::sin(player.transform.rotation.y), 0.f,
                                  glm::cos(player.transform.rotation.y)};

    int x = playerViewDirection.y < 0.5f && playerViewDirection.y > -0.5f
                ? playerChunk.x + int(i % 16) - 8
                : playerChunk.x - int(i % 16) + 8;
    int z = playerViewDirection.z > 0.f ? playerChunk.z + int(i / 16) - 8
                                        : playerChunk.z - int(i / 16) + 8;

    glm::vec3 chunkPosition{x * 32, 0.f, z * 32};

    float chunkDistance =
        glm::length(glm::vec2{chunkPosition.x / 32 - playerChunk.x,
                              chunkPosition.z / 32 - playerChunk.z});

    float angle =
        glm::acos(glm::dot(glm::normalize(playerViewDirection),
                           glm::normalize(chunkPosition / 32.f - playerChunk)));

    if (angle > glm::radians(90.f) && chunkDistance > 2 || chunkDistance > 8)
      continue;

    Chunk chunk = chunkGenerator.generate(chunkPosition);
    {
      std::lock_guard<std::mutex> lock(chunkMutex);
      chunkQueue.push(chunk);
    }
  }
  refreshChunks = true;
}

} // namespace engine

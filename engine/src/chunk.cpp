#include "chunk.hpp"
#include "app.hpp"
#include "collision.hpp"
#include "core/device.hpp"
#include "core/game_object.hpp"
#include "core/model.hpp"
#include <bits/fs_fwd.h>
#include <functional>
#include <glm/common.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/gtc/noise.hpp>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace engine {

Chunk::Chunk(Device &device, std::vector<BlockType> blocks, glm::vec3 &position)
    : GameObject(), blocks{blocks}, device{device} {
  transform.position = position;
  boundingBox = BoxCollider(transform.position, transform.position + 32.f);

  if (blocks.size() != 1)
    calculateMesh();
};

void Chunk::calculateMesh() {
  std::vector<Model::Vertex> vertices;
  std::vector<uint32_t> indices;

  for (int y = 0; y < 32; y++) {
    for (int z = 0; z < 32; z++) {
      for (int x = 0; x < 32; x++) {
        BlockType block = getBlock(x, y, z);
        if (block == BlockType::Air)
          continue;

        BlockType top = getBlock(x, y + 1, z);
        BlockType bottom = getBlock(x, y - 1, z);
        BlockType front = getBlock(x, y, z + 1);
        BlockType back = getBlock(x, y, z - 1);
        BlockType right = getBlock(x + 1, y, z);
        BlockType left = getBlock(x - 1, y, z);

        // Top
        if (top == BlockType::Air) {
          vertices.push_back({{x, y + 1, z}, {1, 0, 0}, {0, 1, 0}});
          vertices.push_back({{x + 1, y + 1, z}, {1, 0, 0}, {0, 1, 0}});
          vertices.push_back({{x + 1, y + 1, z + 1}, {1, 0, 0}, {0, 1, 0}});
          vertices.push_back({{x, y + 1, z + 1}, {1, 0, 0}, {0, 1, 0}});

          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 3);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 1);
        }

        // Bottom
        if (y > 0) {
          if (bottom == BlockType::Air) {
            vertices.push_back({{x, y, z}, {1, 0, 0}, {0, -1, 0}});
            vertices.push_back({{x + 1, y, z}, {1, 0, 0}, {0, -1, 0}});
            vertices.push_back({{x + 1, y, z + 1}, {1, 0, 0}, {0, -1, 0}});
            vertices.push_back({{x, y, z + 1}, {1, 0, 0}, {0, -1, 0}});

            indices.push_back(vertices.size() - 4);
            indices.push_back(vertices.size() - 3);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 4);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 1);
          }
        }

        // Front
        if (front == BlockType::Air) {
          vertices.push_back({{x, y, z + 1}, {1, 0, 0}, {0, 0, 1}});
          vertices.push_back({{x, y + 1, z + 1}, {1, 0, 0}, {0, 0, 1}});
          vertices.push_back({{x + 1, y + 1, z + 1}, {1, 0, 0}, {0, 0, 1}});
          vertices.push_back({{x + 1, y, z + 1}, {1, 0, 0}, {0.f, 0.f, 1}});

          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 3);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 1);
        }

        // Back
        if (back == BlockType::Air) {
          vertices.push_back({{x, y, z}, {1, 0, 0}, {0, 0, -1}});
          vertices.push_back({{x, y + 1, z}, {1, 0, 0}, {0, 0, -1}});
          vertices.push_back({{x + 1, y + 1, z}, {1, 0, 0}, {0, 0, -1}});
          vertices.push_back({{x + 1, y, z}, {1, 0, 0}, {0, 0, -1}});

          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 3);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 1);
        }

        // Left
        if (left == BlockType::Air) {
          vertices.push_back({{x, y, z}, {1, 0, 0}, {-1, 0, 0}});
          vertices.push_back({{x, y + 1, z}, {1, 0, 0}, {-1, 0, 0}});
          vertices.push_back({{x, y + 1, z + 1}, {1, 0, 0}, {-1, 0, 0}});
          vertices.push_back({{x, y, z + 1}, {1, 0, 0}, {-1, 0, 0}});

          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 3);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 1);
        }

        // Right
        if (right == BlockType::Air) {
          vertices.push_back({{x + 1, y, z}, {1, 0, 0}, {1, 0, 0}});
          vertices.push_back({{x + 1, y + 1, z}, {1, 0, 0}, {1, 0, 0}});
          vertices.push_back({{x + 1, y + 1, z + 1}, {1, 0, 0}, {1, 0, 0}});
          vertices.push_back({{x + 1, y, z + 1}, {1, 0, 0}, {1, 0, 0}});

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
  bool isEmpty = true;
  int xPos = static_cast<int>(position.x);
  int yPos = static_cast<int>(position.y);
  int zPos = static_cast<int>(position.z);

  for (int y = 0; y < 32; y++) {
    for (int z = 0; z < 32; z++) {
      for (int x = 0; x < 32; x++) {
        BlockType block;
        block = getBlockType(xPos + x, yPos + y, zPos + z);
        blocks.push_back(block);
        if (block != BlockType::Air)
          isEmpty = false;
      }
    }
  }

  if (isEmpty)
    blocks.resize(1);

  Chunk chunk{device, blocks, position};

  return chunk;
};

int ChunkGenerator::getBlockType(int x, int y, int z) {
  int surfaceY = perlinNoise(x * 0.2f, z * 0.5) * 50;
  surfaceY += perlinNoise(x * 2, z * 0.8) * 10;

  if (y < surfaceY)
    return BlockType::Stone;
  else
    return BlockType::Air;
}

float ChunkGenerator::perlinNoise(int x, int z) {
  float noise = glm::perlin(glm::vec2(x, z) * 0.1f);
  noise = (noise + 1.0f) / 2.0f;

  return noise;
}

void ChunkLoader::startChunkThread(
    GameObject &player, const std::unordered_map<int, Chunk> &chunks) {
  if (!refreshChunks)
    return;

  if (chunkThread.joinable())
    chunkThread.detach();

  chunkThread = std::thread(&ChunkLoader::loadChunks, this, std::ref(player),
                            std::ref(chunks));

  refreshChunks = false;
  stopChunkThread = false;
}

void ChunkLoader::loadChunks(const GameObject &player,
                             const std::unordered_map<int, Chunk> &chunks) {
  glm::vec3 playerChunk{floor(player.transform.position / 32.f)};

  for (int y = 0; y < RENDER_DISTANCE * 2; y++) {
    for (int z = 0; z < RENDER_DISTANCE * 2; z++) {
      for (int x = 0; x < RENDER_DISTANCE * 2; x++) {
        if (stopChunkThread)
          break;

        int xPos = (playerChunk.x + (x - RENDER_DISTANCE)) * 32.f;
        int zPos = (playerChunk.z + (z - RENDER_DISTANCE)) * 32.f;
        int yPos = y * 32.f;

        glm::vec3 chunkPosition{xPos, yPos, zPos};

        if (isLoaded(chunkPosition, chunks))
          continue;

        Chunk chunk = chunkGenerator.generate(chunkPosition);
        {
          std::lock_guard<std::mutex> lock(chunkMutex);
          chunkQueue.push(std::move(chunk));
        }
      }
    }
  }
  refreshChunks = true;
}

void ChunkLoader::unloadOutOfRangeChunks(
    const GameObject &player, std::unordered_map<int, Chunk> &chunks) {
  glm::vec3 playerChunk{glm::floor(player.transform.position / 32.f)};

  auto it = chunks.begin();
  while (it != chunks.end()) {
    const Chunk &chunk = it->second;
    glm::vec3 chunkPosition = chunk.transform.position / 32.f;

    bool isToFar = chunkPosition.x < playerChunk.x - RENDER_DISTANCE ||
                   chunkPosition.x > playerChunk.x + RENDER_DISTANCE - 1 ||
                   chunkPosition.z < playerChunk.z - RENDER_DISTANCE ||
                   chunkPosition.z > playerChunk.z + RENDER_DISTANCE - 1;

    if (isToFar) {
      {
        std::lock_guard<std::mutex> lock(chunkMutex);
        vkQueueWaitIdle(device.graphicsQueue());
        it = chunks.erase(it);
      }
    } else {
      it++;
    }
  }

  stopChunkThread = false;
}

bool ChunkLoader::isLoaded(const glm::vec3 &chunkPosition,
                           const std::unordered_map<int, Chunk> &chunks) {
  int chunkIndex = getChunkIndex(chunkPosition);

  try {
    chunks.at(chunkIndex);
    return true;
  } catch (const std::out_of_range) {
    return false;
  }
}

int ChunkLoader::getChunkIndex(const glm::vec3 &chunkPosition) {
  int index = (chunkPosition.x) + (chunkPosition.z * RENDER_DISTANCE * 2) +
              (chunkPosition.y * RENDER_DISTANCE * RENDER_DISTANCE * 4);
  return index;
}
} // namespace engine

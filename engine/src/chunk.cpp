#include "chunk.hpp"
#include "app.hpp"
#include "block.hpp"
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

using namespace std;

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
        if (y < 31) {
          if (top == BlockType::Air) {
            vertices.push_back(
                {{x, y + 1, z}, {1, 0, 0}, {0, 1, 0}, Model::TexCoord::fourth});
            vertices.push_back({{x + 1, y + 1, z},
                                {1, 1, 1},
                                {0, 1, 0},
                                Model::TexCoord::third});
            vertices.push_back({{x + 1, y + 1, z + 1},
                                {1, 1, 1},
                                {0, 1, 0},
                                Model::TexCoord::first});
            vertices.push_back({{x, y + 1, z + 1},
                                {1, 1, 1},
                                {0, 1, 0},
                                Model::TexCoord::second});

            indices.push_back(vertices.size() - 4);
            indices.push_back(vertices.size() - 3);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 4);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 1);
          }
        } else if (ChunkGenerator::getBlockType(
                       transform.position.x + x, transform.position.y + 32,
                       transform.position.z + z) == BlockType::Air) {
          vertices.push_back(
              {{x, y + 1, z}, {1, 0, 0}, {0, 1, 0}, Model::TexCoord::fourth});
          vertices.push_back({{x + 1, y + 1, z},
                              {1, 1, 1},
                              {0, 1, 0},
                              Model::TexCoord::third});
          vertices.push_back({{x + 1, y + 1, z + 1},
                              {1, 1, 1},
                              {0, 1, 0},
                              Model::TexCoord::first});
          vertices.push_back({{x, y + 1, z + 1},
                              {1, 1, 1},
                              {0, 1, 0},
                              Model::TexCoord::second});

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
            vertices.push_back(
                {{x, y, z}, {1, 1, 1}, {0, -1, 0}, Model::TexCoord::fourth});
            vertices.push_back(
                {{x + 1, y, z}, {1, 1, 1}, {0, -1, 0}, Model::TexCoord::third});
            vertices.push_back({{x + 1, y, z + 1},
                                {1, 1, 1},
                                {0, -1, 0},
                                Model::TexCoord::first});
            vertices.push_back({{x, y, z + 1},
                                {1, 1, 1},
                                {0, -1, 0},
                                Model::TexCoord::second});

            indices.push_back(vertices.size() - 4);
            indices.push_back(vertices.size() - 3);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 4);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 1);
          }
        } else if (ChunkGenerator::getBlockType(
                       transform.position.x + x, transform.position.y - 1,
                       transform.position.z + z) == BlockType::Air) {
          vertices.push_back(
              {{x, y, z}, {1, 1, 1}, {0, -1, 0}, Model::TexCoord::fourth});
          vertices.push_back(
              {{x + 1, y, z}, {1, 1, 1}, {0, -1, 0}, Model::TexCoord::third});
          vertices.push_back({{x + 1, y, z + 1},
                              {1, 1, 1},
                              {0, -1, 0},
                              Model::TexCoord::first});
          vertices.push_back(
              {{x, y, z + 1}, {1, 1, 1}, {0, -1, 0}, Model::TexCoord::second});

          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 3);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 1);
        }

        // Front
        if (z < 31) {
          if (front == BlockType::Air) {
            vertices.push_back(
                {{x, y, z + 1}, {1, 1, 1}, {0, 0, 1}, Model::TexCoord::fourth});
            vertices.push_back({{x, y + 1, z + 1},
                                {1, 1, 1},
                                {0, 0, 1},
                                Model::TexCoord::third});
            vertices.push_back({{x + 1, y + 1, z + 1},
                                {1, 1, 1},
                                {0, 0, 1},
                                Model::TexCoord::first});
            vertices.push_back({{x + 1, y, z + 1},
                                {1, 1, 1},
                                {0.f, 0.f, 1},
                                Model::TexCoord::second});

            indices.push_back(vertices.size() - 4);
            indices.push_back(vertices.size() - 3);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 4);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 1);
          }
        } else if (ChunkGenerator::getBlockType(
                       transform.position.x + x, transform.position.y + y,
                       transform.position.z + 32) == BlockType::Air) {
          vertices.push_back(
              {{x, y, z + 1}, {1, 1, 1}, {0, 0, 1}, Model::TexCoord::fourth});
          vertices.push_back({{x, y + 1, z + 1},
                              {1, 1, 1},
                              {0, 0, 1},
                              Model::TexCoord::third});
          vertices.push_back({{x + 1, y + 1, z + 1},
                              {1, 1, 1},
                              {0, 0, 1},
                              Model::TexCoord::first});
          vertices.push_back({{x + 1, y, z + 1},
                              {1, 1, 1},
                              {0, 0, 1},
                              Model::TexCoord::second});

          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 3);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 1);
        }

        // Back
        if (z > 0) {
          if (back == BlockType::Air) {
            vertices.push_back(
                {{x, y, z}, {1, 1, 1}, {0, 0, -1}, Model::TexCoord::fourth});
            vertices.push_back(
                {{x, y + 1, z}, {1, 1, 1}, {0, 0, -1}, Model::TexCoord::third});
            vertices.push_back({{x + 1, y + 1, z},
                                {1, 1, 1},
                                {0, 0, -1},
                                Model::TexCoord::first});
            vertices.push_back({{x + 1, y, z},
                                {1, 1, 1},
                                {0, 0, -1},
                                Model::TexCoord::second});

            indices.push_back(vertices.size() - 4);
            indices.push_back(vertices.size() - 3);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 4);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 1);
          }
        } else if (ChunkGenerator::getBlockType(
                       transform.position.x + x, transform.position.y + y,
                       transform.position.z - 1) == BlockType::Air) {
          vertices.push_back(
              {{x, y, z}, {1, 1, 1}, {0, 0, -1}, Model::TexCoord::fourth});
          vertices.push_back(
              {{x, y + 1, z}, {1, 1, 1}, {0, 0, -1}, Model::TexCoord::third});
          vertices.push_back({{x + 1, y + 1, z},
                              {1, 1, 1},
                              {0, 0, -1},
                              Model::TexCoord::first});
          vertices.push_back(
              {{x + 1, y, z}, {1, 1, 1}, {0, 0, -1}, Model::TexCoord::second});

          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 3);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 1);
        }

        // Left
        if (x > 0) {
          if (left == BlockType::Air) {
            vertices.push_back(
                {{x, y, z}, {1, 1, 1}, {-1, 0, 0}, Model::TexCoord::fourth});
            vertices.push_back(
                {{x, y + 1, z}, {1, 1, 1}, {-1, 0, 0}, Model::TexCoord::third});
            vertices.push_back({{x, y + 1, z + 1},
                                {1, 1, 1},
                                {-1, 0, 0},
                                Model::TexCoord::first});
            vertices.push_back({{x, y, z + 1},
                                {1, 1, 1},
                                {-1, 0, 0},
                                Model::TexCoord::second});

            indices.push_back(vertices.size() - 4);
            indices.push_back(vertices.size() - 3);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 4);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 1);
          }
        } else if (ChunkGenerator::getBlockType(
                       transform.position.x - 1, transform.position.y + y,
                       transform.position.z + z) == BlockType::Air) {
          vertices.push_back(
              {{x, y, z}, {1, 1, 1}, {-1, 0, 0}, Model::TexCoord::fourth});
          vertices.push_back(
              {{x, y + 1, z}, {1, 1, 1}, {-1, 0, 0}, Model::TexCoord::third});
          vertices.push_back({{x, y + 1, z + 1},
                              {1, 1, 1},
                              {-1, 0, 0},
                              Model::TexCoord::first});
          vertices.push_back(
              {{x, y, z + 1}, {1, 1, 1}, {-1, 0, 0}, Model::TexCoord::second});

          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 3);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 4);
          indices.push_back(vertices.size() - 2);
          indices.push_back(vertices.size() - 1);
        }

        // Right
        if (x < 31) {
          if (right == BlockType::Air) {
            vertices.push_back(
                {{x + 1, y, z}, {1, 1, 1}, {1, 0, 0}, Model::TexCoord::fourth});
            vertices.push_back({{x + 1, y + 1, z},
                                {1, 1, 1},
                                {1, 0, 0},
                                Model::TexCoord::third});
            vertices.push_back({{x + 1, y + 1, z + 1},
                                {1, 1, 1},
                                {1, 0, 0},
                                Model::TexCoord::first});
            vertices.push_back({{x + 1, y, z + 1},
                                {1, 1, 1},
                                {1, 0, 0},
                                Model::TexCoord::second});

            indices.push_back(vertices.size() - 4);
            indices.push_back(vertices.size() - 3);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 4);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 1);
          }
        } else if (ChunkGenerator::getBlockType(
                       transform.position.x + 32, transform.position.y + y,
                       transform.position.z + z) == BlockType::Air) {
          vertices.push_back(
              {{x + 1, y, z}, {1, 1, 1}, {1, 0, 0}, Model::TexCoord::fourth});
          vertices.push_back({{x + 1, y + 1, z},
                              {1, 1, 1},
                              {1, 0, 0},
                              Model::TexCoord::third});
          vertices.push_back({{x + 1, y + 1, z + 1},
                              {1, 1, 1},
                              {1, 0, 0},
                              Model::TexCoord::first});
          vertices.push_back({{x + 1, y, z + 1},
                              {1, 1, 1},
                              {1, 0, 0},
                              Model::TexCoord::second});

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
  chunkThread = std::thread(&ChunkLoader::loadChunks, this, std::ref(player),
                            std::ref(chunks));
}

void ChunkLoader::loadChunks(const GameObject &player,
                             const std::unordered_map<int, Chunk> &chunks) {
  glm::vec3 playerChunk;

  while (running) {
    {
      std::lock_guard<std::mutex> lock(chunkMutex);
      playerChunk = {floor(player.transform.position / 32.f)};
    }

    for (int y = 0; y < RENDER_DISTANCE * 2; y++) {
      for (int z = 0; z < RENDER_DISTANCE * 2; z++) {
        for (int x = 0; x < RENDER_DISTANCE * 2; x++) {
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
  }
}

void ChunkLoader::unloadOutOfRangeChunks(const GameObject &player,
                                         std::unordered_map<int, Chunk> &chunks,
                                         queue<BufferBlock> &freeChunks) {
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
        if (it->second.blocks.size() != 1 ||
            it->second.blocks[0] != BlockType::Air)
          freeChunks.push(it->second.bufferMemory);
        it = chunks.erase(it);
      }
    } else {
      it++;
    }
  }
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
  const int CHUNK_GRID_SIZE = RENDER_DISTANCE * 2;

  int x = static_cast<int>(chunkPosition.x);
  int y = static_cast<int>(chunkPosition.y);
  int z = static_cast<int>(chunkPosition.z);

  return x + (z * CHUNK_GRID_SIZE) + (y * CHUNK_GRID_SIZE * CHUNK_GRID_SIZE);
}
} // namespace engine

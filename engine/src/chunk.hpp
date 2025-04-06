#pragma once
#include "block.hpp"
#include "buffer_block.hpp"
#include "collision.hpp"
#include "core/camera.hpp"
#include "core/device.hpp"
#include "core/game_object.hpp"
#include "core/model.hpp"
#include <atomic>
#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace std;

namespace engine {
struct Position {
  int x;
  int y;
};

class Chunk : public GameObject {
public:
  Chunk(Device &device, std::vector<BlockType> blocks, glm::vec3 &position);
  Chunk(Device &device) : GameObject(), device{device} {
    blocks.resize(32 * 32 * 32, BlockType{0});
  };

  Chunk(Chunk &&) noexcept = default;
  Chunk &operator=(Chunk &&other) noexcept {
    GameObject::operator=(std::move(other));
    chunkMesh = other.chunkMesh;
    blocks = other.blocks;
    return *this;
  }

  void calculateMesh();

  BlockType getBlock(int x, int y, int z) const {
    if (x < 0 || x >= 32 || y < 0 || y >= 32 || z < 0 || z >= 32)
      return BlockType{0};
    if (blocks.size() != 32 * 32 * 32)
      return BlockType{0};
    return blocks[x + z * 32 + y * 32 * 32];
  }

  std::pair<std::vector<Model::Vertex>, std::vector<uint32_t>> &getMesh() {
    return chunkMesh;
  };

  std::vector<Block> getGameObjectSurroundingBlocks(const GameObject &player);

  std::vector<BlockType> blocks;
  BoxCollider boundingBox;
  BufferBlock bufferMemory;

private:
  Device &device;

  std::pair<std::vector<Model::Vertex>, std::vector<uint32_t>> chunkMesh;
  friend class ChunkLoader;
};

class ChunkGenerator {
public:
  ChunkGenerator(Device &device) : device{device} {};
  Chunk generate(glm::vec3 position);

private:
  Device &device;
  static int getBlockType(int x, int y, int z);
  static float perlinNoise(int x, int z);
  friend class Chunk;
};

class ChunkLoader {
public:
  ChunkLoader(Device &device, std::queue<Chunk> &chunkQueue,
              std::queue<int> &chunkUnloaderQueue, std::mutex &chunkMutex)
      : device{device}, chunkMutex{chunkMutex}, chunkGenerator{device},
        chunkQueue{chunkQueue}, chunkUnloaderQueue{chunkUnloaderQueue} {};

  void startChunkThread(GameObject &player,
                        const std::unordered_map<int, Chunk> &chunks);
  void unloadOutOfRangeChunks(const GameObject &player,
                              std::unordered_map<int, Chunk> &chunks,
                              queue<BufferBlock> &freeChunks);
  int getChunkIndex(const glm::vec3 &chunkPosition);

  atomic<bool> running = true;

private:
  Device &device;
  std::thread chunkThread;
  std::mutex &chunkMutex;

  bool isUnloadingChunks = false;
  uint32_t threadId = 0;

  ChunkGenerator chunkGenerator;
  std::queue<Chunk> &chunkQueue;
  std::queue<int> &chunkUnloaderQueue;
  std::vector<Chunk> chunkCache;
  std::vector<Chunk> loadedChunk;
  const int maxChunkQueueSize = 10;

  bool isLoaded(const glm::vec3 &chunkPosition,
                const std::unordered_map<int, Chunk> &chunks);

  void loadChunks(const GameObject &player,
                  const std::unordered_map<int, Chunk> &chunks);

  friend class Generator;
  friend class Chunk;
};
} // namespace engine

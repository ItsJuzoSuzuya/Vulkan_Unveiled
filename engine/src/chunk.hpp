#include "core/device.hpp"
#include "core/game_object.hpp"
#include "core/model.hpp"
#include "heightmap.hpp"
#include <cstdint>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
namespace engine {
struct Position {
  int x;
  int y;
};

struct BlockType {
  uint8_t id;
};

class Chunk {
public:
  Chunk(Device &device, std::vector<BlockType> blocks, glm::vec3 &position);
  void calculateMesh();

  glm::vec3 getPosition() { return position; };
  std::pair<std::vector<Model::Vertex>, std::vector<uint32_t>> getMesh() {
    return chunkMesh;
  };

private:
  Device &device;
  std::vector<BlockType> blocks;

  glm::vec3 position;
  std::pair<std::vector<Model::Vertex>, std::vector<uint32_t>> chunkMesh;

  friend class ChunkGenerator;
};

class ChunkGenerator {
public:
  ChunkGenerator(Device &device) : device{device} {};
  Chunk generate(glm::vec3 position);

private:
  Device &device;
  int getBlockType(int x, int y, int z);
  friend class ChunkLoader;
};

class ChunkLoader {
public:
  ChunkLoader(Device &device, std::queue<Chunk> &chunkQueue,
              std::mutex &chunkMutex, bool &refreshChunks)
      : chunkQueue{chunkQueue}, chunkMutex{chunkMutex},
        refreshChunks{refreshChunks}, chunkGenerator(device) {};

  void startChunkThread(GameObject &player);

private:
  std::thread chunkThread;
  std::mutex &chunkMutex;
  bool &refreshChunks;

  ChunkGenerator chunkGenerator;
  std::queue<Chunk> &chunkQueue;
  std::vector<Chunk> chunkCache;
  std::vector<Chunk> loadedChunk;

  int isChunkCached(const glm::vec3 &chunkPosition);
  bool isChunkLoaded(const glm::vec3 &chunkPosition,
                     std::vector<GameObject> &chunks);
  void loadChunks(GameObject &player);

  friend class Generator;
};
} // namespace engine

#pragma once
#include "chunk.hpp"
#include "core/descriptors.hpp"
#include "core/game_object.hpp"
#include "core/render_system.hpp"
#include <memory>
#include <queue>
#include <thread>
#include <unordered_map>

#include <vector>

namespace engine {

extern int RENDER_DISTANCE;
extern const int WIDTH;
extern const int HEIGHT;

class App {
public:
  App();

  void run();

private:
  Window window{WIDTH, HEIGHT, "Vulkan Application"};
  Device device{window};

  std::unique_ptr<DescriptorPool> descriptorPool;

  void loadGameObjects();

  ChunkGenerator chunkGenerator{device};

  std::thread chunkThread;
  std::mutex queueMutex;

  std::unordered_map<int, Chunk> chunks;
  std::queue<Chunk> chunkQueue;
  std::queue<int> chunkUnloadQueue;
  bool refreshChunks = true;

  bool isChunkLoaded(const glm::vec3 &playerChunkPosition);
  void checkChunks(const glm::vec3 &playerChunk,
                   const glm::vec3 &playerRotation);
  void loadChunks(GameObject &player);

  std::vector<GameObject> gameObjects{};
  bool wakingUp = true;
};
} // namespace engine

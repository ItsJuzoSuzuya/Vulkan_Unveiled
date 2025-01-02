#ifndef APP_HPP
#define APP_HPP
#include "chunk.hpp"
#include "core/descriptors.hpp"
#include "core/game_object.hpp"
#include "core/render_system.hpp"
#include <atomic>
#include <queue>
#include <thread>
#include <vector>

namespace engine {

class App {
public:
  static constexpr int WIDTH = 800;
  static constexpr int HEIGHT = 600;

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

  std::queue<Chunk> chunkQueue;
  std::vector<GameObject> chunkCache;
  bool refreshChunks = true;

  bool isChunkLoaded(const glm::vec3 &playerChunkPosition);
  void checkChunks(const glm::vec3 &playerChunk,
                   const glm::vec3 &playerRotation);
  void loadChunks(GameObject &player);

  std::vector<GameObject> gameObjects;
};
} // namespace engine
#endif

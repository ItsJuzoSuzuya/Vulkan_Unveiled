#ifndef MODEL_HPP
#define MODEL_HPP
#include "buffer.hpp"
#include "device.hpp"
#include "swapchain.hpp"
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace engine {

class Model {
public:
  struct Vertex {
    glm::vec3 position{0.f, 0.f, 0.f};
    glm::vec3 color{0.f, 0.f, 0.f};
    glm::vec3 normal{0.f, 0.f, 0.f};
    glm::vec2 texCoord{0.f, 0.f};

    static std::vector<VkVertexInputBindingDescription>
    getBindingDescriptions();
    static std::vector<VkVertexInputAttributeDescription>
    getAttributeDescriptions();
  };

  struct TexCoord {
    constexpr static glm::vec2 first = glm::vec2{0.f, 0.f};
    constexpr static glm::vec2 second = glm::vec2{0.f, 1.f};
    constexpr static glm::vec2 third = glm::vec2{1.f, 0.f};
    constexpr static glm::vec2 fourth = glm::vec2{1.f, 1.f};
  };

  struct Builder {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    void loadModel(const std::string &filepath);
    Builder &appendModel(const std::vector<Vertex> &vertices,
                         const std::vector<uint32_t> &indices);
  };

  Model(Device &device);
  Model(Device &device, const Model::Builder &builder);
  Model(Device &device, const std::vector<Vertex> &vertices);
  Model(Device &device, const std::vector<Vertex> &vertices,
        const std::vector<uint32_t> &indices);
  ~Model();

  Model(const Model &) = delete;
  Model &operator=(const Model &) = delete;

  std::vector<std::shared_ptr<Buffer>> vertexBuffers;
  std::vector<std::shared_ptr<Buffer>> indexBuffers;

  std::shared_ptr<Buffer> ringBuffer;
  std::shared_ptr<Buffer> stagingBuffer;

  uint32_t vertexCount;
  uint32_t indexCount;

  void writeMeshDataToBuffer(
      const std::pair<std::vector<Vertex>, std::vector<uint32_t>> &mesh,
      uint32_t vertexOffset, uint32_t indexOffset);
  void bind(VkCommandBuffer commandBuffer);
  void draw(VkCommandBuffer commandBuffer, uint32_t instanceCount = 1);

  static std::unique_ptr<Model>
  createModelFromFile(Device &device, const std::string &filepath);
  static std::unique_ptr<Model> loadFromMeshes(
      Device &device,
      std::vector<std::pair<std::vector<Model::Vertex>, std::vector<uint32_t>>>
          &meshes);

private:
  Device &device;

  bool hasIndexBuffer;

  void createVertexBuffer(uint32_t size);
  void createIndexBuffer(uint32_t size);
  void createVertexBuffer(const std::vector<Vertex> &vertices);
  void createIndexBuffer(const std::vector<uint32_t> &indices);
  void createStagingBuffers(uint32_t vertexCount, uint32_t indexCount);
  void createRingBuffer(uint32_t vertexCount, uint32_t indexCount);
};
} // namespace engine
#endif

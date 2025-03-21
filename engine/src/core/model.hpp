#ifndef MODEL_HPP
#define MODEL_HPP
#include "buffer.hpp"
#include "device.hpp"
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
    glm::vec3 position{};
    glm::vec3 color{1.f, 0.f, 0.f};
    glm::vec3 normal{};
    glm::vec2 texCoord{};

    static std::vector<VkVertexInputBindingDescription>
    getBindingDescriptions();
    static std::vector<VkVertexInputAttributeDescription>
    getAttributeDescriptions();
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
  std::vector<std::shared_ptr<Buffer>> stagingBuffers;

  uint32_t vertexCount;
  uint32_t indexCount;

  void writeMeshDataToBuffers(
      const std::pair<std::vector<Vertex>, std::vector<uint32_t>> &mesh,
      uint32_t vertexOffset, uint32_t indexOffset, uint32_t frameIndex);
  void bind(VkCommandBuffer commandBuffer, int frameIndex);
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
};
} // namespace engine
#endif

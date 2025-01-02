#ifndef MODEL_HPP
#define MODEL_HPP
#include "buffer.hpp"
#include "device.hpp"
#include <cstdint>
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

    static std::vector<VkVertexInputBindingDescription>
    getBindingDescriptions();
    static std::vector<VkVertexInputAttributeDescription>
    getAttributeDescriptions();
  };

  struct Builder {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    void loadModel(const std::string &filepath);
  };

  Model(Device &device, const Model::Builder &builder);
  Model(Device &device, const std::vector<Vertex> &vertices);
  Model(Device &device, const std::vector<Vertex> &vertices,
        const std::vector<uint32_t> &indices);
  ~Model();

  Model(const Model &) = delete;
  Model &operator=(const Model &) = delete;

  std::unique_ptr<Buffer> vertexBuffer;
  uint32_t vertexCount;

  void bind(VkCommandBuffer commandBuffer);
  void draw(VkCommandBuffer commandBuffer, uint32_t instanceCount = 1);

  static std::unique_ptr<Model>
  createModelFromFile(Device &device, const std::string &filepath);

private:
  Device &device;

  uint32_t indexCount;
  bool hasIndexBuffer;
  std::unique_ptr<Buffer> indexBuffer;

  void createVertexBuffer(const std::vector<Vertex> &vertices);
  void createIndexBuffer(const std::vector<uint32_t> &indices);
};
} // namespace engine
#endif

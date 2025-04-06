#include "model.hpp"
#include "device.hpp"
#include "swapchain.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <glm/ext/vector_float3.hpp>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tinygltf/tiny_gltf.h>

using namespace std;

namespace engine {
std::vector<VkVertexInputBindingDescription>
Model::Vertex::getBindingDescriptions() {
  std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
  bindingDescriptions[0].binding = 0;
  bindingDescriptions[0].stride = sizeof(Vertex);
  bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription>
Model::Vertex::getAttributeDescriptions() {
  std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {};
  attributeDescriptions.push_back(
      {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
  attributeDescriptions.push_back(
      {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)});
  attributeDescriptions.push_back(
      {2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
  attributeDescriptions.push_back(
      {3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord)});

  return attributeDescriptions;
}

void Model::Builder::loadModel(const std::string &filepath) {
  tinygltf::Model gltfModel;
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string warn;

  bool result = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filepath);

  if (!warn.empty()) {
    std::cout << "Warning: " << warn << std::endl;
  }

  if (!err.empty()) {
    std::cout << "Error: " << err << std::endl;
  }

  if (!result) {
    std::cout << "Failed to load glTF model!" << std::endl;
    return;
  } else {
    std::cout << "Loaded glTF model: " << filepath << std::endl;
  }

  for (const auto &mesh : gltfModel.meshes) {
    for (const auto &primitive : mesh.primitives) {
      const auto &vertexAccessor =
          gltfModel.accessors[primitive.attributes.at("POSITION")];
      const auto &vertexBufferView =
          gltfModel.bufferViews[vertexAccessor.bufferView];
      const auto &vertexBuffer = gltfModel.buffers[vertexBufferView.buffer];
      const float *vertexData = reinterpret_cast<const float *>(
          &vertexBuffer
               .data[vertexBufferView.byteOffset + vertexAccessor.byteOffset]);

      const auto &normalAccessor =
          gltfModel.accessors[primitive.attributes.at("NORMAL")];
      const auto &normalBufferView =
          gltfModel.bufferViews[normalAccessor.bufferView];
      const auto &normalBuffer = gltfModel.buffers[normalBufferView.buffer];
      const float *normalData = reinterpret_cast<const float *>(
          &normalBuffer
               .data[normalBufferView.byteOffset + normalAccessor.byteOffset]);

      const auto &uvAccessor =
          gltfModel.accessors[primitive.attributes.at("TEXCOORD_0")];
      const auto &uvBufferView = gltfModel.bufferViews[uvAccessor.bufferView];
      const auto &uvBuffer = gltfModel.buffers[uvBufferView.buffer];
      const float *uvData = reinterpret_cast<const float *>(
          &uvBuffer.data[uvBufferView.byteOffset + uvAccessor.byteOffset]);

      for (size_t i = 0; i < vertexAccessor.count; i++) {
        Vertex vertex{};
        vertex.position = {vertexData[i * 3], vertexData[i * 3 + 1],
                           vertexData[i * 3 + 2]};
        vertex.normal = {normalData[i * 3], normalData[i * 3 + 1],
                         normalData[i * 3 + 2]};
        vertex.texCoord = {uvData[i * 2], uvData[i * 2 + 1]};
        vertices.push_back(vertex);
      }

      if (primitive.indices >= 0) {
        const auto &indexAccessor = gltfModel.accessors[primitive.indices];
        const auto &indexBufferView =
            gltfModel.bufferViews[indexAccessor.bufferView];
        const auto &indexBuffer = gltfModel.buffers[indexBufferView.buffer];
        const uint32_t *indexData = reinterpret_cast<const uint32_t *>(
            &indexBuffer.data[0] + indexBufferView.byteOffset +
            indexAccessor.byteOffset);

        for (size_t i = 0; i < indexAccessor.count; i++) {
          indices.push_back(indexData[i]);
        }
      }
    }
  }
}

Model::Builder &
Model::Builder::appendModel(const std::vector<Vertex> &vertices,
                            const std::vector<uint32_t> &indices) {
  this->vertices.insert(this->vertices.end(), vertices.begin(), vertices.end());
  this->indices.insert(this->indices.end(), indices.begin(), indices.end());

  return *this;
}

std::unique_ptr<Model> Model::createModelFromFile(Device &device,
                                                  const std::string &filepath) {
  Builder builder = {};
  builder.loadModel(filepath);

  return std::make_unique<Model>(device, builder);
}

std::unique_ptr<Model> Model::loadFromMeshes(
    Device &device,
    std::vector<std::pair<std::vector<Model::Vertex>, std::vector<uint32_t>>>
        &meshes) {

  Builder builder = {};
  for (auto &mesh : meshes) {
    builder.appendModel(mesh.first, mesh.second);
  }

  return std::make_unique<Model>(device, builder);
}

Model::Model(Device &device) : device{device} {
  cout << "Creating model with default constructor" << endl;
  createRingBuffer(10000000, 15000000);
  createStagingBuffers(10000000, 15000000);
}

Model::Model(Device &device, const Model::Builder &builder) : device{device} {
  createVertexBuffer(builder.vertices);
  createIndexBuffer(builder.indices);
}

Model::Model(Device &device, const std::vector<Vertex> &vertices)
    : device{device} {
  createVertexBuffer(vertices);
}

Model::Model(Device &device, const std::vector<Vertex> &vertices,
             const std::vector<uint32_t> &indices)
    : device{device} {
  createVertexBuffer(vertices);
  createIndexBuffer(indices);
}

void Model::createRingBuffer(uint32_t vertexCount, uint32_t indexCount) {
  size_t instanceSize =
      vertexCount * sizeof(Model::Vertex) + indexCount * sizeof(uint32_t);
  ringBuffer = std::make_shared<Buffer>(
      device, instanceSize, SwapChain::MAX_FRAMES_IN_FLIGHT,
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  hasIndexBuffer = indexCount > 0;
}

void Model::createVertexBuffer(uint32_t vertexCount) {
  uint32_t vertexSize = sizeof(Model::Vertex);
  for (uint32_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
    auto vertexBuffer = std::make_shared<Buffer>(
        device, vertexSize, vertexCount,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vertexBuffers.push_back(vertexBuffer);
  }
}

void Model::createVertexBuffer(const std::vector<Vertex> &vertices) {
  vertexCount = vertices.size();
  assert(vertexCount >= 3 && "Vertex count must be at least 3!");

  uint32_t vertexSize = sizeof(vertices[0]);
  VkDeviceSize bufferSize = vertexSize * vertexCount;

  Buffer stagingBuffer{device, vertexSize, vertexCount,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

  stagingBuffer.map();
  stagingBuffer.writeToBuffer((void *)vertices.data());

  vertexBuffers[0] = std::make_unique<Buffer>(
      device, vertexSize, vertexCount,
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  device.copyBuffer(stagingBuffer.getBuffer(), vertexBuffers[0]->getBuffer(),
                    bufferSize);
}

Model::~Model() {}

void Model::createIndexBuffer(uint32_t indexCount) {
  hasIndexBuffer = indexCount > 0;

  if (!hasIndexBuffer)
    return;

  uint32_t indexSize = sizeof(uint32_t);

  for (uint32_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
    auto indexBuffer = std::make_shared<Buffer>(
        device, indexSize, indexCount,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    indexBuffers.push_back(indexBuffer);
  }
}
void Model::createIndexBuffer(const std::vector<uint32_t> &indices) {
  indexCount = static_cast<uint32_t>(indices.size());
  hasIndexBuffer = indexCount > 0;

  if (!hasIndexBuffer)
    return;

  uint32_t indexSize = sizeof(indices[0]);
  VkDeviceSize bufferSize = indexSize * indexCount;

  Buffer stagingBuffer{device, indexSize, indexCount,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

  stagingBuffer.map();
  stagingBuffer.writeToBuffer((void *)indices.data());

  indexBuffers[0] = std::make_unique<Buffer>(
      device, indexSize, indexCount,
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  device.copyBuffer(stagingBuffer.getBuffer(), indexBuffers[0]->getBuffer(),
                    bufferSize);
}

void Model::createStagingBuffers(uint32_t vertexCount, uint32_t indexCount) {
  uint32_t vertexSize = sizeof(Model::Vertex);
  uint32_t indexSize = sizeof(uint32_t);

  size_t vertexBufferSize = vertexSize * vertexCount + indexSize * indexCount;
  stagingBuffer = std::make_shared<Buffer>(
      device, vertexBufferSize, SwapChain::MAX_FRAMES_IN_FLIGHT,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  stagingBuffer->map();
}

void Model::writeMeshDataToBuffer(
    const std::pair<std::vector<Vertex>, std::vector<uint32_t>> &mesh,
    uint32_t vertexBufferOffset, uint32_t indexBufferOffset) {
  size_t vertexDataSize = mesh.first.size() * sizeof(Vertex);
  size_t indexDataSize = mesh.second.size() * sizeof(uint32_t);

  stagingBuffer->writeToBuffer((void *)mesh.first.data(), vertexDataSize,
                               vertexBufferOffset);
  stagingBuffer->writeToBuffer((void *)mesh.second.data(), indexDataSize,
                               indexBufferOffset + 10000000 * sizeof(Vertex));
}

void Model::bind(VkCommandBuffer commandBuffer) {
  VkBuffer vkRingBuffers[] = {ringBuffer->getBuffer()};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vkRingBuffers, offsets);

  if (hasIndexBuffer) {
    uint32_t indexBufferOffset = 10000000 * sizeof(Vertex);

    vkCmdBindIndexBuffer(commandBuffer, ringBuffer->getBuffer(),
                         indexBufferOffset, VK_INDEX_TYPE_UINT32);
  }
}

void Model::draw(VkCommandBuffer commandBuffer, uint32_t instanceCount) {
  if (hasIndexBuffer) {
    vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, 0, 0, 0);
  } else {
    vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
  }
}
} // namespace engine

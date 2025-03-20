#pragma once
#include "buffer.hpp"
#include "camera.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace engine {
struct FrameInfo {
  int frameIndex;
  VkCommandBuffer commandBuffer;
  Camera camera;
  VkDescriptorSet &descriptorSet;
  std::shared_ptr<Buffer> drawCallBuffer;
  std::shared_ptr<Buffer> objectDataBuffer;
};
} // namespace engine

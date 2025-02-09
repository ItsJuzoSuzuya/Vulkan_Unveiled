#pragma once
#include "camera.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>

namespace engine {
struct FrameInfo {
  int frameIndex;
  VkCommandBuffer commandBuffer;
  Camera camera;
  VkDescriptorSet descriptorSet;
};
} // namespace engine

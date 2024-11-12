#include "camera.hpp"
#include <vulkan/vulkan_core.h>

namespace engine {
struct FrameInfo {
  int frameIndex;
  VkCommandBuffer commandBuffer;
  Camera camera;
  VkDescriptorSet descriptorSet;
};
} // namespace engine

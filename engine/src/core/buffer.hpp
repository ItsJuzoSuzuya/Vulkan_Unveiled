#ifndef BUFFER_HPP
#define BUFFER_HPP
#include "device.hpp"
#include <cstdint>
#include <vulkan/vulkan_core.h>
namespace engine {

class Buffer {
public:
  Buffer(Device &device, VkDeviceSize instanceSize, uint32_t instanceCount,
         VkBufferUsageFlags usageFlags,
         VkMemoryPropertyFlags memoryPropertyFlags,
         VkDeviceSize minOffsetAlignment = 0);
  ~Buffer();

  Buffer(const Buffer &) = delete;
  Buffer &operator=(const Buffer &) = delete;

  VkBuffer getBuffer() { return buffer; }

  VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
  void writeToBuffer(void *data, VkDeviceSize size = VK_WHOLE_SIZE,
                     VkDeviceSize offset = 0);

private:
  VkBuffer buffer = VK_NULL_HANDLE;
  VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
  void *mappedMemory = nullptr;

  Device &device;
  VkDeviceSize instanceSize;
  uint32_t instanceCount;
  VkBufferUsageFlags usageFlags;
  VkMemoryPropertyFlags memoryPropertyFlags;

  VkDeviceSize alignmentSize;
  VkDeviceSize bufferSize;

  VkDeviceSize getAlignment(VkDeviceSize instanceSize,
                            VkDeviceSize minOffsetAlignment);
  void unmap();
};
} // namespace engine
#endif

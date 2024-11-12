#include "buffer.hpp"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vulkan/vulkan_core.h>
namespace engine {

VkDeviceSize Buffer::getAlignment(VkDeviceSize instanceSize,
                                  VkDeviceSize minOffsetAlignment) {
  if (minOffsetAlignment == 0)
    return instanceSize;

  return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
}

Buffer::Buffer(Device &device, VkDeviceSize instanceSize,
               uint32_t instanceCount, VkBufferUsageFlags usageFlags,
               VkMemoryPropertyFlags memoryPropertyFlags,
               VkDeviceSize minOffsetAlignment)
    : device{device}, instanceSize{instanceSize}, usageFlags{usageFlags},
      memoryPropertyFlags{memoryPropertyFlags} {
  alignmentSize = getAlignment(instanceSize, minOffsetAlignment);
  bufferSize = alignmentSize * instanceCount;
  device.createBuffer(bufferSize, usageFlags, memoryPropertyFlags, buffer,
                      bufferMemory);
}

Buffer::~Buffer() {
  unmap();
  vkDestroyBuffer(device.device(), buffer, nullptr);
  vkFreeMemory(device.device(), bufferMemory, nullptr);
}

VkResult Buffer::map(VkDeviceSize size, VkDeviceSize offset) {
  assert(buffer && bufferMemory && "Buffer has to be created before mapping!");
  return vkMapMemory(device.device(), bufferMemory, offset, size, 0,
                     &mappedMemory);
}

void Buffer::unmap() {
  if (mappedMemory) {
    vkUnmapMemory(device.device(), bufferMemory);
    mappedMemory = nullptr;
  };
}

void Buffer::writeToBuffer(void *data, VkDeviceSize size, VkDeviceSize offset) {
  assert(mappedMemory && "Cannot write to unmapped memory!");

  if (size == VK_WHOLE_SIZE) {
    memcpy(mappedMemory, data, bufferSize);
  } else {
    char *memOffset = (char *)mappedMemory;
    memOffset += offset;
    memcpy(memOffset, data, size);
  }
}

void Buffer::flush(VkDeviceSize size, VkDeviceSize offset) {
  VkMappedMemoryRange mappedRange = {};
  mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  mappedRange.size = size;
  mappedRange.offset = offset;
  mappedRange.memory = bufferMemory;
  vkFlushMappedMemoryRanges(device.device(), 1, &mappedRange);
}

VkDescriptorBufferInfo Buffer::descriptorInfo() {
  VkDescriptorBufferInfo bufferInfo = {};
  bufferInfo.offset = 0;
  bufferInfo.range = bufferSize;
  bufferInfo.buffer = buffer;
  return bufferInfo;
}

} // namespace engine

#include "heightmap.hpp"
#include "core/buffer.hpp"
#include <cmath>
#include <cstdint>
#include <glm/gtc/noise.hpp>

#include <iostream>
#include <vulkan/vulkan_core.h>
namespace engine {

HeightMap::HeightMap(Device &device, const uint32_t width,
                     const uint32_t height)
    : device{device}, width{width}, height{height} {}

void HeightMap::generate(int maxHeight) {
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      float noise = perlinNoise(x, y);
      noise = (noise + 1.0f) / 2.0f;
      heightMap.push_back(static_cast<int>(std::round(noise * maxHeight)));
    }
  }
}

HeightMap::~HeightMap() {
  // vkDestroySampler(device.device(), sampler, nullptr);
  // vkDestroyImageView(device.device(), imageView, nullptr);
  // vkDestroyImage(device.device(), image, nullptr);
  // vkFreeMemory(device.device(), imageMemory, nullptr);
}

void HeightMap::createImage() {
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = VK_FORMAT_R8_UNORM;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage =
      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  device.createImageWithInfo(imageInfo, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT, image,
                             imageMemory);
}

void HeightMap::createImageView() {
  VkImageViewCreateInfo imageViewInfo{};
  imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  imageViewInfo.image = image;
  imageViewInfo.format = VK_FORMAT_R8_UNORM;
  imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageViewInfo.subresourceRange.baseMipLevel = 0;
  imageViewInfo.subresourceRange.levelCount = 1;
  imageViewInfo.subresourceRange.baseArrayLayer = 0;
  imageViewInfo.subresourceRange.layerCount = 1;

  vkCreateImageView(device.device(), &imageViewInfo, nullptr, &imageView);
}

void HeightMap::createSampler() {
  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = 1.0f;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;

  vkCreateSampler(device.device(), &samplerInfo, nullptr, &sampler);

  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = imageView;
  imageInfo.sampler = sampler;
}

void HeightMap::copyHeightMapData() {
  VkDeviceSize instanceSize = sizeof(heightMap[0]);
  uint32_t heightMapSize = width * height;

  std::cout << "HeightMap size: " << heightMapSize << std::endl;
  std::cout << "Instance size: " << instanceSize << std::endl;

  Buffer stagingBuffer{device, instanceSize, heightMapSize,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

  stagingBuffer.map();
  stagingBuffer.writeToBuffer((void *)heightMap.data());

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  device.copyBufferToImage(stagingBuffer.getBuffer(), image, region);
}

float HeightMap::perlinNoise(float x, float y) {
  float noise = glm::perlin(glm::vec2(x, y) * 0.1f);
  noise = (noise + 1.0f) / 2.0f;

  return noise;
}

void HeightMap::print() {
  for (int i = 0; i < heightMap.size(); i++) {
    if (i % 16 == 0) {
      std::cout << std::endl;
    }
    std::cout << heightMap[i] << " ";
  }
  std::cout << std::endl;
}

} // namespace engine

#include "core/device.hpp"
#include <glm/detail/qualifier.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/vec2.hpp>
#include <vulkan/vulkan_core.h>
namespace engine {

class HeightMap {
public:
  HeightMap(Device &device, uint32_t width, uint32_t height);
  ~HeightMap();

  void generate(int maxHeight);
  static float perlinNoise(float x, float y);

  std::vector<int> &getHeightMap() { return heightMap; }
  int getHeight(int x, int y) const { return heightMap[x + y * width]; }

  void copyHeightMapData();
  void createImage();
  void createImageView();
  void createSampler();

  VkDescriptorImageInfo *getImageInfo() { return &imageInfo; }

  void print();

private:
  Device &device;
  const uint32_t width;
  const uint32_t height;

  std::vector<int> heightMap;

  VkImage image;
  VkDescriptorImageInfo imageInfo;
  VkDeviceMemory imageMemory;
  VkImageView imageView;
  VkSampler sampler;
};

} // namespace engine

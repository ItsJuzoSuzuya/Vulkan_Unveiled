#ifndef DEVICE_HPP
#define DEVICE_HPP
#include "window.hpp"
#include <cstdint>
#include <optional>
#include <vector>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace engine {

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;
  bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

struct SwapchainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

class Device {
public:
  Device(Window &window);
  ~Device();

  Device(const Device &) = delete;
  Device &operator=(const Device &) = delete;
  Device(const Device &&) = delete;
  Device &operator=(const Device &&) = delete;

  VkSurfaceKHR surface() { return surface_; }
  VkDevice device() { return device_; }
  VkCommandPool getCommandPool() { return commandPool; }
  VkQueue graphicsQueue() { return graphicsQueue_; }
  VkQueue presentQueue() { return presentQueue_; }

  SwapchainSupportDetails getSwapChainSupport() {
    return querySwapChainSupport(physicalDevice);
  }

  QueueFamilyIndices findQueueFamilies() {
    return findQueueFamilies(physicalDevice);
  }

  VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates,
                               VkImageTiling tiling,
                               VkFormatFeatureFlags features);

  void createImageWithInfo(const VkImageCreateInfo &imageInfo,
                           VkMemoryPropertyFlags properties, VkImage &image,
                           VkDeviceMemory &imageMemory);

  void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties, VkBuffer &buffer,
                    VkDeviceMemory &bufferMemory);

  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size,
                  VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);
  void copyModel(VkBuffer srcBuffer, VkBuffer vertexBuffer,
                 VkBuffer indexBuffer, VkDeviceSize vertexSize,
                 VkDeviceSize indexSize);

  void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                         uint32_t height);
  void copyImageToBuffer(VkCommandBuffer &commandBuffer, VkBuffer dstBuffer,
                         VkImage image, VkBufferImageCopy region);
  void transitionImageLayout(VkImage image, VkFormat format,
                             VkImageLayout oldLayout, VkImageLayout newLayout);
  void transitionDepthImage(VkCommandBuffer commandBuffer, VkImage image,
                            VkImageLayout oldLayout, VkImageLayout newLayout);

  VkCommandBuffer allocateCommandBuffer(VkCommandBufferLevel level);
  VkCommandBuffer beginSingleTimeCommands();
  void submitCommands(VkCommandBuffer &commandBuffer);
  void endSingleTimeCommands(VkCommandBuffer &commandBuffer);

private:
#ifdef NDEBUG
  const bool enableValidationLayers = false;
#else
  const bool enableValidationLayers = true;
#endif

  void createInstance();
  void setupDebugMessenger();
  void createSurface();
  void pickPhysicalDevice();
  void createLogicalDevice();
  void createCommandPool();

  std::vector<const char *> getRequiredExtensions();

  bool checkValidationLayerSupport();
  void populateDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT &createInfo);

  bool isDeviceSuitable(VkPhysicalDevice device);
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
  bool checkDeviceExtensionSupport(VkPhysicalDevice device);
  SwapchainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

  uint32_t findMemoryType(uint32_t typeFilter,
                          VkMemoryPropertyFlags properties);

  VkInstance instance;
  VkDebugUtilsMessengerEXT debugMessenger;
  Window &window;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkPhysicalDeviceProperties properties;
  VkCommandPool commandPool;

  VkDevice device_;
  VkSurfaceKHR surface_;
  VkQueue graphicsQueue_;
  VkQueue presentQueue_;

  const std::vector<const char *> validationLayers = {
      "VK_LAYER_KHRONOS_validation"};
  const std::vector<const char *> deviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_EXT_MULTI_DRAW_EXTENSION_NAME,
      VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME};
};
} // namespace engine
#endif

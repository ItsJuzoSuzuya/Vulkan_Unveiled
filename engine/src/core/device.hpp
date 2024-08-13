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

struct SwapChainSupportDetails {
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
  operator VkDevice() { return device_; }

  SwapChainSupportDetails getSwapChainSupport() {
    return querySwapChainSupport(physicalDevice);
  }

  QueueFamilyIndices findQueueFamilies() {
    return findQueueFamilies(physicalDevice);
  }

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
  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

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
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};
} // namespace engine

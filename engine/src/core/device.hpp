#include "window.hpp"
#include <vector>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace engine {
class Device {
public:
  Device(Window &window);
  ~Device();

private:
#ifdef NDEBUG
  const bool enableValidationLayers = false;
#else
  const bool enableValidationLayers = true;
#endif

  void createInstance();
  void setupDebugMessenger();
  void createSurface();

  std::vector<const char *> getRequiredExtensions();

  bool checkValidationLayerSupport();
  void populateDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT &createInfo);

  VkInstance instance;
  VkDebugUtilsMessengerEXT debugMessenger;
  Window &window;

  VkSurfaceKHR surface_;

  const std::vector<const char *> validationLayers = {
      "VK_LAYER_KHRONOS_validation"};
};
} // namespace engine

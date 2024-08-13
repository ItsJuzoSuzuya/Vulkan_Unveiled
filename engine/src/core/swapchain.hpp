#include "device.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>

namespace engine {
class SwapChain {
public:
  SwapChain(Device &device, VkExtent2D extent);
  ~SwapChain();

  SwapChain(const SwapChain &) = delete;
  SwapChain &operator=(const SwapChain &) = delete;

private:
  Device &device;
  VkExtent2D windowExtent;

  VkSwapchainKHR swapChain;
  std::vector<VkImage> swapChainImages;

  void createSwapChain();
  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);
  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
};
} // namespace engine

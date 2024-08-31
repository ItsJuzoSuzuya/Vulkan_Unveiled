#ifndef SWAPCHAIN_HPP
#define SWAPCHAIN_HPP
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

  VkRenderPass getRenderPass() { return renderPass; }

private:
  Device &device;
  VkExtent2D windowExtent;

  VkSwapchainKHR swapChain;
  VkFormat swapChainImageFormat;
  VkFormat swapChainDepthFormat;
  VkExtent2D swapChainExtent;
  std::vector<VkImage> swapChainImages;
  std::vector<VkImageView> swapChainImageViews;

  VkRenderPass renderPass;

  std::vector<VkImage> depthImages;
  std::vector<VkDeviceMemory> depthImageMemories;
  std::vector<VkImageView> depthImageViews;

  std::vector<VkFramebuffer> framebuffers;

  void createSwapChain();
  void createImageViews();
  void createRenderPass();
  void createDepthResources();
  void createFramebuffers();

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);
  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
  VkFormat findDepthFormat();
};
} // namespace engine
#endif

#ifndef SWAPCHAIN_HPP
#define SWAPCHAIN_HPP
#include "device.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>

namespace engine {
class SwapChain {
public:
  static const int MAX_FRAMES_IN_FLIGHT = 2;

  SwapChain(Device &device, VkExtent2D extent);
  ~SwapChain();

  SwapChain(const SwapChain &) = delete;
  SwapChain &operator=(const SwapChain &) = delete;

  VkRenderPass getRenderPass() { return renderPass; }
  VkFramebuffer getFrameBuffer(int index) { return framebuffers[index]; }
  VkExtent2D extent() { return swapChainExtent; }

  VkResult acquireNextImage(uint32_t *imageIndex);
  VkResult submitCommandBuffer(const VkCommandBuffer *commandBuffer,
                               uint32_t *imageIndex);

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

  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;

  size_t currentFrame = 0;

  void createSwapChain();
  void createImageViews();
  void createRenderPass();
  void createDepthResources();
  void createFramebuffers();
  void createSyncObjects();

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);
  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
  VkFormat findDepthFormat();
};
} // namespace engine
#endif

#pragma once
#include "model.hpp"
#include "pipeline.hpp"
#include "swapchain.hpp"
#include "window.hpp"
#include <memory>
#include <vulkan/vulkan_core.h>
namespace engine {
class RenderSystem {
public:
  RenderSystem(Device &device, Window &window,
               VkDescriptorSetLayout descriptorSetLayout);
  ~RenderSystem();

  RenderSystem(const RenderSystem &) = delete;
  RenderSystem &operator=(const RenderSystem &) = delete;

  VkCommandBuffer beginFrame();
  VkCommandBuffer getCurrentCommandBuffer() const {
    return commandBuffers[currentFrameIndex];
  }
  void recordCommandBuffer(VkCommandBuffer commandBuffer,
                           std::shared_ptr<Model> model);
  void endFrame();

private:
  Device &device;
  Window &window;
  VkPipelineLayout pipelineLayout;
  std::unique_ptr<Pipeline> pipeline;
  std::unique_ptr<SwapChain> swapChain;
  std::vector<VkCommandBuffer> commandBuffers;

  void createPipelineLayout(VkDescriptorSetLayout descriptorSetLayout);
  void createPipeline(VkRenderPass renderPass);
  void createCommandBuffers();

  void recreateSwapChain();

  uint32_t currentImageIndex;
  int currentFrameIndex = 0;
};
} // namespace engine

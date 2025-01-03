#ifndef RENDERSYSTEM_HPP
#define RENDERSYSTEM_HPP
#include "frame_info.hpp"
#include "game_object.hpp"
#include "pipeline.hpp"
#include "swapchain.hpp"
#include "window.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>
namespace engine {
class RenderSystem {
public:
  RenderSystem(Device &device, Window &window,
               VkDescriptorSetLayout descriptorSetLayout);
  ~RenderSystem();

  RenderSystem(const RenderSystem &) = delete;
  RenderSystem &operator=(const RenderSystem &) = delete;

  int getFrameIndex() const { return currentFrameIndex; }

  VkCommandBuffer beginFrame();
  VkCommandBuffer getCurrentCommandBuffer() const {
    return commandBuffers[currentFrameIndex];
  }
  float getAspectRatio() const { return swapChain->extentAspectRatio(); }
  void recordCommandBuffer(VkCommandBuffer commandBuffer);
  void renderWorld(FrameInfo &frameInfo, uint32_t renderDistance,
                   GameObject &originCube);
  void renderGameObjects(FrameInfo &frameInfo,
                         std::vector<GameObject> &gameObjects);
  void endRenderPass(VkCommandBuffer commandBuffer);
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
#endif

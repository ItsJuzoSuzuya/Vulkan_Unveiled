#pragma once
#include "pipeline.hpp"
#include <memory>
#include <vulkan/vulkan_core.h>
namespace engine {
class RenderSystem {
public:
  RenderSystem(Device &device, VkRenderPass renderPass,
               VkDescriptorSetLayout descriptorSetLayout);
  ~RenderSystem();

  RenderSystem(const RenderSystem &) = delete;
  RenderSystem &operator=(const RenderSystem &) = delete;

private:
  Device &device;
  VkPipelineLayout pipelineLayout;
  std::unique_ptr<Pipeline> pipeline;

  void createPipelineLayout(VkDescriptorSetLayout descriptorSetLayout);
  void createPipeline(VkRenderPass renderPass);
};
} // namespace engine

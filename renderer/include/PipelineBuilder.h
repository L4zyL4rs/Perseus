#pragma once
#include "PipelineManager.h"
#include <vulkan/vulkan_core.h>

class PipelineBuilder {
  RenderContext *context;
  Swapchain *swapchain;
  PipelineManager *manager;
  PipelineBundle bundle;
  PipelineType type;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  VkPipelineRasterizationStateCreateInfo rasterizer{};
  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  VkPipelineMultisampleStateCreateInfo multisampling{};
  VkPipelineViewportStateCreateInfo viewportState{};
  std::vector<VkDynamicState> dynamicStates;
  VkPipelineRenderingCreateInfo renderingInfo{};
  VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  VkPushConstantRange pushConstant{};
  uint32_t pushConstantRangeCount = 0;

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  std::vector<VkVertexInputAttributeDescription> vertexInputAttributes;
  std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  VkVertexInputBindingDescription vertexBinding{};

  std::string debugLabel{};

public:
  void setDefaults(PipelineManager *m, PipelineType t);
  // Blend mode for correctly rendering (partially) transparent objects
  // INCLUDING TEXT!!!!
  void setBlendModeAlpha();
  void enableDepthWrite();
  void disableDepthWrite();
  void disableDepthTest();
  void disableCulling();
  void enablePushConstants(size_t size, VkShaderStageFlags stageFlag);
  VkShaderModule createShaderModule(const std::vector<char> &code);
  void addVertShader(const std::string &file);
  void addGeomShader(const std::string &file);
  void addFragShader(const std::string &file);
  void
  setVertexFormat(VkVertexInputBindingDescription binding,
                  std::vector<VkVertexInputAttributeDescription> attributes);
  void setNoVertexInput();
  void addDescriptor(VkDescriptorSetLayout layout);
  void setPipelineDebugLabel(std::string label);
  void setAttachmentFormats(uint32_t attachmentCount, VkFormat *colorFormats);
  void setAttachmentFormats(uint32_t attachmentCount, VkFormat *colorFormats,
                            VkFormat stencilFormat, VkFormat depthFormat);
  void build();
};

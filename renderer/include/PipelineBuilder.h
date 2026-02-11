#pragma once
#include "PipelineManager.h"


class PipelineBuilder {
	RenderContext* context;
	Swapchain* swapchain;
	PipelineManager* manager;
	PipelineBundle bundle;
	PipelineType type;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	VkPipelineMultisampleStateCreateInfo multisampling{};
	VkPipelineViewportStateCreateInfo viewportState{};
	std::vector<VkDynamicState> dynamicStates;
	VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	VkPushConstantRange pushConstant{};
	uint32_t pushConstantRangeCount = 0;

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	std::vector<VkVertexInputAttributeDescription> vertexInputAttributes;
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	VkVertexInputBindingDescription vertexBinding{};

public:
	void setDefaults(PipelineManager* m, PipelineType t);
	// Blend mode for correctly rendering (partially) transparent objects
	// INCLUDING TEXT!!!!
	void setBlendModeAlpha();
	void enableDepthWrite();
	void enablePushConstants(size_t size);
	VkShaderModule createShaderModule(const std::vector<char>& code);
	void addVertShader(const std::string& file);
	void addGeomShader(const std::string& file);
	void addFragShader(const std::string& file);
	void setVertexFormat(VkVertexInputBindingDescription binding, std::vector<VkVertexInputAttributeDescription> attributes);
	void addDescriptor(VkDescriptorSetLayout layout);
	void build();
};
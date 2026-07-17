#pragma once
#include <Swapchain.h>
#include <unordered_map>

// This could probably just be header-only, no idea why I decided to write a cpp file too

struct PipelineBundle {
	// Pipeline specific objects
	VkPipeline pipeline;
	VkPipelineLayout layout;
	//std::vector<VkDescriptorSetLayout> descriptorSetLayout;
};

enum class PipelineType {
	None,
	Mesh,
	Text,
  Skybox
};

// Define array to be able to iterate over all PipelineTypes
// Weird way to do this if there are more PipelineTypes
static const PipelineType allPipelineTypes[] = { PipelineType::Mesh, PipelineType::Text, PipelineType::Skybox};

// Define hash function for unordered_map of pipelines
template <>
struct std::hash<PipelineType> {
	size_t operator()(const PipelineType& type) const {
		return std::hash<int>()(static_cast<int>(type));
	}
};

class PipelineManager {
public:
	PipelineManager(RenderContext* c, Swapchain* s);
	RenderContext* context = nullptr;
	Swapchain* swapchain = nullptr;
	std::unordered_map<PipelineType, size_t> pushConstantSizes;
	void addPipeline(PipelineType type, const PipelineBundle& bundle);
	const PipelineBundle& getPipelineBundleP(PipelineType type);
	const VkPipeline& getPipelineP(PipelineType type);
	const VkPipeline& pipeline(PipelineType type);
	const VkPipelineLayout getPipelineLayout(PipelineType type);
	void cleanup();

private:
	std::unordered_map<PipelineType, PipelineBundle> pipelines;
	void destroyPipelineBundle(PipelineType type);
};


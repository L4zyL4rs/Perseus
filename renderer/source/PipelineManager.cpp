#include "PipelineManager.h"

// Links the given PipelineBundle to the specified type (later material)
void PipelineManager::addPipeline(PipelineType type, const PipelineBundle& bundle) {
	if (pipelines.find(type) != pipelines.end()) {
		destroyPipelineBundle(type);
	}
	pipelines[type] = bundle;
}

const PipelineBundle& PipelineManager::getPipelineBundleP(PipelineType type) {
	return pipelines.at(type);
}

const VkPipeline& PipelineManager::getPipelineP(PipelineType type) {
	return pipelines.at(type).pipeline;
}

const VkPipeline& PipelineManager::pipeline(PipelineType type) {
	return pipelines.at(type).pipeline;
}

const VkPipelineLayout PipelineManager::getPipelineLayout(PipelineType type) {
	return pipelines.at(type).layout;
}

void PipelineManager::destroyPipelineBundle(PipelineType type) {
	// Descriptor sets are destroyed in DescriptorAllocator for now
	vkDestroyPipelineLayout(context->device, pipelines[type].layout, 0);
	vkDestroyPipeline(context->device, pipelines[type].pipeline, 0);
}

void PipelineManager::cleanup() {
	std::cout << "\nCleaning pipelines";
	for (auto& type : allPipelineTypes) {
		destroyPipelineBundle(type);
	}
}

PipelineManager::PipelineManager(RenderContext* c, Swapchain* s) : context(c), swapchain(s) {}

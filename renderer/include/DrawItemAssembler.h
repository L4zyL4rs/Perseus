#pragma once
#include "AssetManager.h"
#include "PipelineManager.h"

// No lookup anymore, everything needed to draw is in here
// Only vertices are loaded globally
// Transform will be uploaded as push constant
struct DrawItem {
	uint32_t meshStartIndex;
	uint32_t meshStopIndex;
	VkBuffer meshBuffer;
	VkBuffer indexBuffer;
	PipelineType pipeline;
	std::vector<VkDescriptorSet> descriptorSets;
	glm::mat4 transform;
	uint64_t sortKey;	// Worry about draw sorting later
};

inline std::ostream& operator<<(std::ostream& out, const DrawItem& in) {
    return out  << "MeshStartIndex = " << in.meshStartIndex << "\n"
                << "MeshStopIndex = " << in.meshStopIndex << "\n"
                << "SortKey = " << in.sortKey << "\n";
}

// Do not put this here maybe xd
struct MeshPushConstant {
	glm::mat4 transform;
};

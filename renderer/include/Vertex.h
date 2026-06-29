#pragma once
#include "glm_config.h"
#include "DescriptorAllocator.h"

struct Vertex {
    glm::vec3 pos{ 0.0f };
    glm::vec3 color{ 1.0f };
    glm::vec2 texCoord{ 0.0f };

    bool operator==(const Vertex& other) const;
    static VkVertexInputBindingDescription getBindingDescription();

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
};

namespace std {
    template<> struct hash<Vertex> {
		inline size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
		}
    };
};


#pragma once
#include "VulkanHelper.h"

struct DescriptorSetLayouts {
    VkDescriptorSetLayout camera{};
    VkDescriptorSetLayout meshAndSampler{};
    VkDescriptorSetLayout fontSampler{};
    VkDescriptorSetLayout shadows{};
};

class DescriptorAllocator {
public:
    VkDescriptorPool pool{};
    DescriptorSetLayouts layouts{};
    RenderContext* context = nullptr;
    uint32_t maxObjects{};
    uint32_t* MAX_FRAMES_IN_FLIGHT{};

    DescriptorAllocator(RenderContext* c, uint32_t mO, uint32_t* MFIF);
    void reset();
    void allocate(VkDescriptorSetLayout layout, VkDescriptorSet *pDescriptorSet, size_t count, std::string = "");

private:
    void createDescriptorSetLayouts();
    void createCameraDescriptorSetLayout();
    void createMeshAndSamplerDescriptorSetLayout();
    void createFontDescriptorSetLayout();
    void createShadowDescriptorSetLayout();
    void destroyDescriptorSetLayouts();
    void createDescriptorPool();
    void createDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding> bindings, VkDescriptorSetLayout* pLayout);
};

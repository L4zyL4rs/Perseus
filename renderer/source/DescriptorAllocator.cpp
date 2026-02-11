#include "DescriptorAllocator.h"
#include <array>

void DescriptorAllocator::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 4> poolSizes{};
    // Descriptor for global variables like camera position etc
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(*MAX_FRAMES_IN_FLIGHT);

    // Object local descriptors
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(*MAX_FRAMES_IN_FLIGHT * maxObjects);
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[2].descriptorCount = static_cast<uint32_t>(*MAX_FRAMES_IN_FLIGHT * maxObjects);

    // Descriptor for texture Atlas
    poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[3].descriptorCount = static_cast<uint32_t>(*MAX_FRAMES_IN_FLIGHT);


    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(*MAX_FRAMES_IN_FLIGHT * maxObjects);

    if (vkCreateDescriptorPool(context->device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void DescriptorAllocator::reset() {
    // Use when new scene is loaded or program is cleaned up
    vkDestroyDescriptorPool(context->device, pool, nullptr);
    destroyDescriptorSetLayouts();
}

VkDescriptorSet DescriptorAllocator::allocate(VkDescriptorSetLayout layout, VkDescriptorSet* pDescriptorSet, size_t count) {
    std::vector<VkDescriptorSetLayout> layouts(*MAX_FRAMES_IN_FLIGHT, layout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = count;
    allocInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(context->device, &allocInfo, pDescriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }
}

void DescriptorAllocator::destroyDescriptorSetLayouts() {
    vkDestroyDescriptorSetLayout(context->device, layouts.camera, nullptr);
    vkDestroyDescriptorSetLayout(context->device, layouts.meshAndSampler, nullptr);
    vkDestroyDescriptorSetLayout(context->device, layouts.fontSampler, nullptr);
}

void DescriptorAllocator::createDescriptorSetLayouts() {
    createCameraDescriptorSetLayout();
    createMeshAndSamplerDescriptorSetLayout();
    createFontDescriptorSetLayout();
}

void DescriptorAllocator::createCameraDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding globalUboLayoutBinding{};
    globalUboLayoutBinding.binding = 0;
    globalUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    globalUboLayoutBinding.descriptorCount = 1;
    globalUboLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL; // VK_SHADER_STAGE_VERTEX_BIT || VK_SHADER_STAGE_FRAGMENT_BIT;
    globalUboLayoutBinding.pImmutableSamplers = nullptr; //Optional

    std::array<VkDescriptorSetLayoutBinding, 1> globalBindings = { globalUboLayoutBinding };
    VkDescriptorSetLayoutCreateInfo globalLayoutInfo{};
    globalLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    globalLayoutInfo.bindingCount = static_cast<uint32_t>(globalBindings.size());
    globalLayoutInfo.pBindings = globalBindings.data();

    if (vkCreateDescriptorSetLayout(context->device, &globalLayoutInfo, nullptr, &(layouts.camera)) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

// RENAME THIS TO JUST SAMPLER
void DescriptorAllocator::createMeshAndSamplerDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 1> localBindings = { samplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo localLayoutInfo{};
    localLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    localLayoutInfo.bindingCount = static_cast<uint32_t>(localBindings.size());
    localLayoutInfo.pBindings = localBindings.data();

    if (vkCreateDescriptorSetLayout(context->device, &localLayoutInfo, nullptr, &(layouts.meshAndSampler)) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

//void DescriptorAllocator::createMeshAndSamplerDescriptorSetLayout() {
//    VkDescriptorSetLayoutBinding localUboLayoutBinding{};
//    localUboLayoutBinding.binding = 1;
//    localUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//    localUboLayoutBinding.descriptorCount = 1;
//    localUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
//    localUboLayoutBinding.pImmutableSamplers = nullptr; //Optional
//
//    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
//    samplerLayoutBinding.binding = 2;
//    samplerLayoutBinding.descriptorCount = 1;
//    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//    samplerLayoutBinding.pImmutableSamplers = nullptr;
//    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
//
//    std::array<VkDescriptorSetLayoutBinding, 2> localBindings = { localUboLayoutBinding, samplerLayoutBinding };
//    VkDescriptorSetLayoutCreateInfo localLayoutInfo{};
//    localLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
//    localLayoutInfo.bindingCount = static_cast<uint32_t>(localBindings.size());
//    localLayoutInfo.pBindings = localBindings.data();
//
//    if (vkCreateDescriptorSetLayout(context->device, &localLayoutInfo, nullptr, &(layouts.meshAndSampler)) != VK_SUCCESS) {
//        throw std::runtime_error("failed to create descriptor set layout!");
//    }
//}

void DescriptorAllocator::createFontDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding fontSamplerLayoutBinding = {};
    fontSamplerLayoutBinding.binding = 0;
    fontSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    fontSamplerLayoutBinding.descriptorCount = 1;
    fontSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    fontSamplerLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &fontSamplerLayoutBinding;

    vkCreateDescriptorSetLayout(context->device, &layoutInfo, nullptr, &(layouts.fontSampler));
}

DescriptorAllocator::DescriptorAllocator(RenderContext* c, uint32_t mO, uint32_t* MFIF)
    : context(c)
    , maxObjects(mO)
    , MAX_FRAMES_IN_FLIGHT(MFIF){
    createDescriptorPool();
    createDescriptorSetLayouts();
}
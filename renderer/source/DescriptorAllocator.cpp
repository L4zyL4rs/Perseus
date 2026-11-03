#include "DescriptorAllocator.h"
#include <array>
#include <vulkan/vulkan_core.h>

void DescriptorAllocator::createDescriptorPool() {
  std::array<VkDescriptorPoolSize, 6> poolSizes{};
  // Descriptor for global variables like camera position etc
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = static_cast<uint32_t>(*MAX_FRAMES_IN_FLIGHT);

  // Object local descriptors
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[1].descriptorCount =
      static_cast<uint32_t>(*MAX_FRAMES_IN_FLIGHT * maxObjects);
  poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[2].descriptorCount =
      static_cast<uint32_t>(*MAX_FRAMES_IN_FLIGHT * maxObjects);

  // Descriptor for texture Atlas
  poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[3].descriptorCount = static_cast<uint32_t>(*MAX_FRAMES_IN_FLIGHT);

  // Shadow pass descriptors
  poolSizes[4].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[4].descriptorCount = 10; // Make this into MAX_SHADOW_COUNT later
  poolSizes[5].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[5].descriptorCount = 10; // Make this into MAX_SHADOW_COUNT later

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = static_cast<uint32_t>(*MAX_FRAMES_IN_FLIGHT * maxObjects);

  if (vkCreateDescriptorPool(context->device, &poolInfo, nullptr, &pool) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void DescriptorAllocator::reset() {
  // Use when new scene is loaded or program is cleaned up
  vkDestroyDescriptorPool(context->device, pool, nullptr);
  destroyDescriptorSetLayouts();
}

void DescriptorAllocator::allocate(VkDescriptorSetLayout layout,
                                   VkDescriptorSet *pDescriptorSet,
                                   size_t count, std::string name) {
  std::vector<VkDescriptorSetLayout> layoutsVector(count, layout);
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = pool;
  allocInfo.descriptorSetCount = count;
  allocInfo.pSetLayouts = layoutsVector.data();

  if (vkAllocateDescriptorSets(context->device, &allocInfo, pDescriptorSet) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }
  context->setDebugLabel(VK_OBJECT_TYPE_DESCRIPTOR_SET, *pDescriptorSet, name);
}

void DescriptorAllocator::destroyDescriptorSetLayouts() {
  vkDestroyDescriptorSetLayout(context->device, layouts.camera, nullptr);
  vkDestroyDescriptorSetLayout(context->device, layouts.meshAndSampler,
                               nullptr);
  vkDestroyDescriptorSetLayout(context->device, layouts.fontSampler, nullptr);
}

void DescriptorAllocator::createDescriptorSetLayouts() {
  createCameraDescriptorSetLayout();
  createMeshAndSamplerDescriptorSetLayout();
  createFontDescriptorSetLayout();
  createShadowDescriptorSetLayout();
}

void DescriptorAllocator::createDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding> bindings, VkDescriptorSetLayout* pLayout) {
  VkDescriptorSetLayoutCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  info.bindingCount = bindings.size();
  info.pBindings = bindings.data();

  if(vkCreateDescriptorSetLayout(context->device, &info, nullptr, pLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create descriptor set layout!");
  }
}

void DescriptorAllocator::createCameraDescriptorSetLayout() {
  VkDescriptorSetLayoutBinding globalUboLayoutBinding{};
  globalUboLayoutBinding.binding = 0;
  globalUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  globalUboLayoutBinding.descriptorCount = 1;
  globalUboLayoutBinding.stageFlags =
      VK_SHADER_STAGE_ALL; // VK_SHADER_STAGE_VERTEX_BIT ||
                           // VK_SHADER_STAGE_FRAGMENT_BIT;
  globalUboLayoutBinding.pImmutableSamplers = nullptr; // Optional

  std::vector<VkDescriptorSetLayoutBinding> globalBindings = {
      globalUboLayoutBinding};

  createDescriptorSetLayout(globalBindings, &layouts.camera);
}

// RENAME THIS TO JUST SAMPLER
void DescriptorAllocator::createMeshAndSamplerDescriptorSetLayout() {
  VkDescriptorSetLayoutBinding samplerLayoutBinding{};
  samplerLayoutBinding.binding = 1;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.pImmutableSamplers = nullptr;
  samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  std::vector<VkDescriptorSetLayoutBinding> localBindings = {
      samplerLayoutBinding};

  createDescriptorSetLayout(localBindings, &layouts.meshAndSampler);
}

void DescriptorAllocator::createShadowDescriptorSetLayout() {
  VkDescriptorSetLayoutBinding shadowLayoutBinding{};
  shadowLayoutBinding.binding = 0;
  shadowLayoutBinding.descriptorCount = 1;
  shadowLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  shadowLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  createDescriptorSetLayout({shadowLayoutBinding}, &layouts.shadows);
}

  // void DescriptorAllocator::createMeshAndSamplerDescriptorSetLayout() {
  //     VkDescriptorSetLayoutBinding localUboLayoutBinding{};
  //     localUboLayoutBinding.binding = 1;
  //     localUboLayoutBinding.descriptorType =
  //     VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  //     localUboLayoutBinding.descriptorCount = 1;
  //     localUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  //     localUboLayoutBinding.pImmutableSamplers = nullptr; //Optional
  //
  //     VkDescriptorSetLayoutBinding samplerLayoutBinding{};
  //     samplerLayoutBinding.binding = 2;
  //     samplerLayoutBinding.descriptorCount = 1;
  //     samplerLayoutBinding.descriptorType =
  //     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  //     samplerLayoutBinding.pImmutableSamplers = nullptr;
  //     samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  //
  //     std::array<VkDescriptorSetLayoutBinding, 2> localBindings = {
  //     localUboLayoutBinding, samplerLayoutBinding };
  //     VkDescriptorSetLayoutCreateInfo localLayoutInfo{};
  //     localLayoutInfo.sType =
  //     VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  //     localLayoutInfo.bindingCount =
  //     static_cast<uint32_t>(localBindings.size()); localLayoutInfo.pBindings
  //     = localBindings.data();
  //
  //     if (vkCreateDescriptorSetLayout(context->device, &localLayoutInfo,
  //     nullptr, &(layouts.meshAndSampler)) != VK_SUCCESS) {
  //         throw std::runtime_error("failed to create descriptor set
  //         layout!");
  //     }
  // }

  void DescriptorAllocator::createFontDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding fontSamplerLayoutBinding = {};
    fontSamplerLayoutBinding.binding = 0;
    fontSamplerLayoutBinding.descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    fontSamplerLayoutBinding.descriptorCount = 1;
    fontSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    fontSamplerLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &fontSamplerLayoutBinding;

    vkCreateDescriptorSetLayout(context->device, &layoutInfo, nullptr,
                                &(layouts.fontSampler));
  }

  DescriptorAllocator::DescriptorAllocator(RenderContext * c, uint32_t mO,
                                           uint32_t *MFIF)
      : context(c),
        maxObjects(mO),
        MAX_FRAMES_IN_FLIGHT(MFIF) {
    createDescriptorPool();
    createDescriptorSetLayouts();
  }

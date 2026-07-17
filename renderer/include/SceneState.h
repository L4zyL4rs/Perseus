#include "DescriptorAllocator.h"
#include "Light.h"
#include <vulkan/vulkan_core.h>

struct SceneStateUBO {
  glm::vec4 cameraPos{};
  glm::vec4 cameraUp{};
  glm::vec4 cameraFront{};
  glm::mat4 viewMatrix{};
  glm::mat4 projMatrix{};

  glm::ivec4 lightCount;
  LightSource lights[10]{};
};

// Make this something like a generator for the global descriptor set that sets
// up camera, light, whatever
class SceneState {
  SceneStateUBO ubo;
  std::vector<VkDescriptorSet> descriptors{};
  std::vector<VkBuffer> buffers{};
  std::vector<VkDeviceMemory> buffersMemory{};
  std::vector<void *> buffersMapped{};
  DescriptorAllocator &descriptorAllocator;
  uint32_t &MAX_FRAMES_IN_FLIGHT;
  uint32_t currentFrame{};

public:
  SceneState(DescriptorAllocator &da)
      : descriptorAllocator(da),
        MAX_FRAMES_IN_FLIGHT(*descriptorAllocator.MAX_FRAMES_IN_FLIGHT) {
    createBuffers();
    createDescriptors();
  }

  void setCamera(glm::vec3 pos, glm::vec3 up, glm::vec3 front) {
    ubo.cameraPos = glm::vec4(pos, 0);
    ubo.cameraUp = glm::vec4(up, 0);
    ubo.cameraFront = glm::vec4(front, 0);
  }

  void setLights(uint32_t count, LightSource *lights) {
    ubo.lightCount.x = count;
    memcpy(ubo.lights, lights, count * sizeof(LightSource));
  }

  void setView(glm::mat4 viewMatrix) { ubo.viewMatrix = viewMatrix; }
  void setProj(glm::mat4 projMatrix) { ubo.projMatrix = projMatrix; }

  void submit() {
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    memcpy(buffersMapped[currentFrame], &ubo, sizeof(SceneStateUBO));
  }

  void createBuffers() {

    VkDeviceSize bufferSize = sizeof(SceneStateUBO);

    buffers.resize(MAX_FRAMES_IN_FLIGHT);
    buffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    buffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      VulkanHelper::createBuffer(descriptorAllocator.context, bufferSize,
                                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                 &buffers[i], &buffersMemory[i]);

      vkMapMemory(descriptorAllocator.context->device, buffersMemory[i], 0,
                  bufferSize, 0, &buffersMapped[i]);
    }
  }

  void createDescriptors() {
    descriptors.resize(MAX_FRAMES_IN_FLIGHT);
    descriptorAllocator.allocate(descriptorAllocator.layouts.camera,
                                 &descriptors[0],
                                 *descriptorAllocator.MAX_FRAMES_IN_FLIGHT);
    VkDescriptorBufferInfo uniformBufferInfo{};

    // Descriptors only need to be updated once, this just tells the descriptor
    // what memory region it describes
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      uniformBufferInfo.buffer = buffers[i];
      uniformBufferInfo.offset = 0;
      uniformBufferInfo.range = sizeof(SceneStateUBO);

      std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
      descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[0].dstSet = descriptors[i];
      descriptorWrites[0].dstBinding = 0;
      descriptorWrites[0].dstArrayElement = 0;
      descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrites[0].descriptorCount = 1;
      descriptorWrites[0].pBufferInfo = &uniformBufferInfo;
      vkUpdateDescriptorSets(descriptorAllocator.context->device,
                             static_cast<uint32_t>(descriptorWrites.size()),
                             descriptorWrites.data(), 0, nullptr);
    }
  }

  VkDescriptorSet getDescriptor() { return descriptors[currentFrame]; }
};

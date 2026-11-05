#include "FrameManager.h"
#include "PipelineBuilder.h"
#include "PipelineManager.h"
#include "SkyPass.h"
#include <complex>
#include <iterator>
#include <regex>
#include <utility>
#include <vulkan/vulkan_core.h>

FrameManager::FrameManager(Swapchain *s)
    : swapchain(s),
      context(swapchain->context),
      window(context->window),
      pipelineManager(context, swapchain),
      commandPool(context),
      MAX_FRAMES_IN_FLIGHT(2),
      descriptorAllocator(context, MAX_OBJECTS, &MAX_FRAMES_IN_FLIGHT),
      assetManager(context, &commandPool, &descriptorAllocator),
      objectManager(&descriptorAllocator, &pipelineManager, &commandPool,
                    &assetManager, &MAX_FRAMES_IN_FLIGHT),
      fontRenderer(context, &descriptorAllocator, &pipelineManager,
                   &assetManager, &commandPool),
      imageManager(swapchain->imageManager),
      meshPass(imageManager),
      skyPass(imageManager),
      shadowPass(imageManager),
      sceneState(descriptorAllocator)

{
  createCommandBuffers();
  createSyncObjects();
  assetManager.createBuffers();
  setupRenderpasses();
  MAIN_LOOP_RUNNING = true;
  // physicsThread = std::thread(&ObjectManager::physics, &objectManager,
  // std::ref(MAIN_LOOP_RUNNING));
}

void FrameManager::cleanup() {
  MAIN_LOOP_RUNNING = false;
  physicsThread.join();
  descriptorAllocator.reset();
  pipelineManager.cleanup();
  objectManager.cleanup();
  fontRenderer.cleanup();
  vkDestroySampler(context->device, shadowSampler, nullptr); 

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(context->device, imageAvailableSemaphores[i], nullptr);
    vkDestroySemaphore(context->device, renderFinishedSemaphores[i], nullptr);
    vkDestroyFence(context->device, inFlightFences[i], nullptr);
  }
}

void FrameManager::createCommandBuffers() {
  commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool.get();
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

  if (vkAllocateCommandBuffers(context->device, &allocInfo,
                               commandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}

void FrameManager::recordCommandBuffer(VkCommandBuffer commandBuffer,
                                       uint32_t imageIndex) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;                  // Optional
  beginInfo.pInheritanceInfo = nullptr; // Optional
  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }
  //std::cout << "Current frame: " << currentFrame
  //          << "\nImage Index: " << imageIndex << "\n";
  // transitionLayoutRender(imageIndex);

  transitionLayoutRender(imageIndex);
  
  shadowPass.execute(commandBuffer, pipelineManager);
  imageManager.sync(commandBuffer, shadow, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
  skyPass.execute(commandBuffer, pipelineManager);
  meshPass.execute(commandBuffer, pipelineManager);

  resolve(commandBuffer, imageIndex);

  // transitionLayoutPresent(imageIndex);

  // std::cout << "Command buffer error " << vkEndCommandBuffer(commandBuffer)
  // << "\n";

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }
}

void FrameManager::draw(FrameRenderInfo& renderInfo) {
  //meshPass.setDrawItems(&renderInfo.meshItems);

  //std::cout << "Found extent of " << swapchain->extent.height << " x "
  //          << swapchain->extent.width << "\n";

  vkWaitForFences(context->device, 1, &inFlightFences[currentFrame], VK_TRUE,
                  UINT64_MAX);

  uint32_t imageIndex; // SPECIFIES WHICH IMAGE IS DRAWN TO AND PRESENTED
  VkResult result = vkAcquireNextImageKHR(
      context->device, swapchain->swapChain, UINT64_MAX,
      imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    std::cout << "Recreating Swapchain!\n";
    swapchain->recreateSwapChain();
    imageManager.recreateSwapchain();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  sceneState.setCamera(renderInfo.camPos, renderInfo.camUp, renderInfo.camForward);
  sceneState.setView(renderInfo.viewMatrix);
  sceneState.setProj(renderInfo.projMatrix);
  sceneState.setLights(static_cast<uint32_t>(renderInfo.lights.size()), renderInfo.lights.data());
  sceneState.submit(currentFrame);
  assembleDrawItems(renderInfo);
  skyPass.setCamera(renderInfo.camForward, renderInfo.camUp, renderInfo.verticalFOV, renderInfo.horizontalFOV);
  skyPass.setSunPosition(glm::normalize(renderInfo.lights[0].pos));

  vkResetFences(context->device, 1, &inFlightFences[currentFrame]);
  vkResetCommandBuffer(commandBuffers[currentFrame], 0);
  recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

  VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (vkQueueSubmit(context->graphicsQueue, 1, &submitInfo,
                    inFlightFences[currentFrame]) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
  }

  VkSwapchainKHR swapChains[] = {swapchain->swapChain};

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.pResults = nullptr; // Optional

  result = vkQueuePresentKHR(context->getPresentQueue(), &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      window->framebufferResized) {
    window->framebufferResized = false;
    swapchain->recreateSwapChain();
    imageManager.recreateSwapchain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void FrameManager::createSyncObjects() {
  imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(context->device, &semaphoreInfo, nullptr,
                          &imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(context->device, &semaphoreInfo, nullptr,
                          &renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(context->device, &fenceInfo, nullptr,
                      &inFlightFences[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create semaphores!");
    }
  }
}

void FrameManager::drawDrawItems(const std::vector<DrawItem> &items,
                                 VkCommandBuffer commandBuffer,
                                 uint32_t currentFrame) {
  for (auto &item : items) {
    // Just always rebind everything for now like a caveman
    // Driver should probably make this a no-op but idk
    VkDeviceSize offsets[] = {0};
    // std::cout << item.meshBuffer
    std::cout << "Item with " << item.descriptorSets.size()
              << " descriptor sets\n";
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      pipelineManager.pipeline(item.pipeline));
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &item.meshBuffer, offsets);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineManager.getPipelineLayout(item.pipeline), 0,
                            item.descriptorSets.size(),
                            item.descriptorSets.data(), 0, 0);
    MeshPushConstant pushConstant{item.transform};
    if (pipelineManager.pushConstantSizes[item.pipeline] != 0) {
      vkCmdPushConstants(
          commandBuffer, pipelineManager.getPipelineLayout(item.pipeline),
          VK_SHADER_STAGE_VERTEX_BIT, 0,
          pipelineManager.pushConstantSizes[item.pipeline], &pushConstant);
    }

    if (item.indexBuffer == nullptr) {
      std::cout << "Drawing non-indexed\n" << item;
      vkCmdDraw(commandBuffer, item.meshStopIndex - item.meshStartIndex + 1, 1,
                item.meshStartIndex, 0);
    } else {
      vkCmdBindIndexBuffer(commandBuffer, item.indexBuffer, 0,
                           VK_INDEX_TYPE_UINT32);
      std::cout << "Start index " << item.meshStartIndex << "\n";
      vkCmdDrawIndexed(commandBuffer,
                       item.meshStopIndex - item.meshStartIndex + 1, 1,
                       item.meshStartIndex, 0, 0);
    }
  }
}

void FrameManager::assembleDrawItems(FrameRenderInfo& renderInfo) {
  drawItems.clear();
  meshPass.clearDrawItems();
  shadowPass.clearDrawItems();
  DrawItem item;
  const VkBuffer &vertexBuffer = assetManager.getVertexBuffer();
  const VkBuffer &indexBuffer = assetManager.getIndexBuffer();

  std::vector<VkDescriptorSet> descriptors(3);
  for(auto& obj : renderInfo.meshes) {
    const MeshResource &mesh = assetManager.getMesh(obj.mesh);
    descriptors[0] = sceneState.getDescriptor(currentFrame);
    descriptors[1] = assetManager.getDescriptors(mesh.texture)[currentFrame];
    descriptors[2] = shadowDescriptor;
    
    item.descriptorSets = descriptors;
    item.transform = obj.transform;
    item.indexBuffer = indexBuffer;
    item.meshBuffer = vertexBuffer;
    item.meshStartIndex = mesh.start;
    item.meshStopIndex = mesh.end;
    item.sortKey = 0;

    if(obj.flags.castsShadow) {
      DrawItem shadowItem = item;
      shadowItem.pipeline = PipelineType::Depth;
      shadowItem.descriptorSets = {descriptors[0]};
      shadowPass.addDrawItem(shadowItem);
    }
    
    item.pipeline = PipelineType::Mesh;
    meshPass.addDrawItem(item);
  }

  drawItems.append_range(fontRenderer.assembleDrawItems(renderInfo.texts, currentFrame));
}


// EVERYTHING BELOW IS FRAMEGRAPH LOGIC AND SHOULD BE ITS OWN CLASS ONE DAY

void FrameManager::transitionLayoutPresent(uint32_t imageIndex) {
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = swapchain->images[imageIndex];
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  vkCmdPipelineBarrier(commandBuffers[currentFrame],
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);
}

void FrameManager::transitionLayoutRender(uint32_t imageIndex) {
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = swapchain->images[imageIndex];
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  vkCmdPipelineBarrier(commandBuffers[currentFrame],
                       VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);

}

void FrameManager::resolve(VkCommandBuffer cmd, uint32_t imageIndex) {
  // Transition color image GENERAL -> TRANSFER_SRC_OPTIMAL
  VkImageMemoryBarrier2 colorBarrier{};
  colorBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  colorBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  colorBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
  colorBarrier.dstStageMask = VK_PIPELINE_STAGE_2_RESOLVE_BIT;
  colorBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
  colorBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
  colorBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  colorBarrier.image = meshPass.getOutputImage();
  colorBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  colorBarrier.subresourceRange.baseMipLevel = 0;
  colorBarrier.subresourceRange.levelCount = 1;
  colorBarrier.subresourceRange.baseArrayLayer = 0;
  colorBarrier.subresourceRange.layerCount = 1;

  // Transition swapchain image GENERAL -> TRANSFER_DST_OPTIMAL
  VkImageMemoryBarrier2 swapBarrier{};
  swapBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  swapBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  swapBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
  swapBarrier.dstStageMask = VK_PIPELINE_STAGE_2_RESOLVE_BIT;
  swapBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
  swapBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
  swapBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  swapBarrier.image = swapchain->images[imageIndex];
  swapBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  swapBarrier.subresourceRange.baseMipLevel = 0;
  swapBarrier.subresourceRange.levelCount = 1;
  swapBarrier.subresourceRange.baseArrayLayer = 0;
  swapBarrier.subresourceRange.layerCount = 1;

  std::array<VkImageMemoryBarrier2, 2> barriers = {colorBarrier, swapBarrier};

  VkDependencyInfo depInfo{};
  depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  depInfo.imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size());
  depInfo.pImageMemoryBarriers = barriers.data();
  vkCmdPipelineBarrier2(cmd, &depInfo);

  VkImageResolve region{};
  region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.srcSubresource.mipLevel = 0;
  region.srcSubresource.baseArrayLayer = 0;
  region.srcSubresource.layerCount = 1;
  region.srcOffset = {0, 0, 0};
  region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.dstSubresource.mipLevel = 0;
  region.dstSubresource.baseArrayLayer = 0;
  region.dstSubresource.layerCount = 1;
  region.dstOffset = {0, 0, 0};
  region.extent = {swapchain->extent.width, swapchain->extent.height, 1};

  vkCmdResolveImage(cmd, meshPass.getOutputImage(),
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    swapchain->images[imageIndex],
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  VkImageMemoryBarrier2 colorRestoreBarrier{};
  colorRestoreBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  colorRestoreBarrier.srcStageMask = VK_PIPELINE_STAGE_2_RESOLVE_BIT;
  colorRestoreBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
  colorRestoreBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  colorRestoreBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT |
                                      VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
  colorRestoreBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  colorRestoreBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
  colorRestoreBarrier.image = meshPass.getOutputImage();
  colorRestoreBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  colorRestoreBarrier.subresourceRange.baseMipLevel = 0;
  colorRestoreBarrier.subresourceRange.levelCount = 1;
  colorRestoreBarrier.subresourceRange.baseArrayLayer = 0;
  colorRestoreBarrier.subresourceRange.layerCount = 1;

  // Transition swapchain image TRANSFER_DST_OPTIMAL -> PRESENT_SRC_KHR
  VkImageMemoryBarrier2 presentBarrier{};
  presentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  presentBarrier.srcStageMask = VK_PIPELINE_STAGE_2_RESOLVE_BIT;
  presentBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
  presentBarrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
  presentBarrier.dstAccessMask = VK_ACCESS_2_NONE;
  presentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  presentBarrier.image = swapchain->images[imageIndex];
  presentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  presentBarrier.subresourceRange.baseMipLevel = 0;
  presentBarrier.subresourceRange.levelCount = 1;
  presentBarrier.subresourceRange.baseArrayLayer = 0;
  presentBarrier.subresourceRange.layerCount = 1;

  VkDependencyInfo presentDepInfo{};
  presentDepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  std::array<VkImageMemoryBarrier2, 2> finalBarriers = {colorRestoreBarrier,
                                                        presentBarrier};
  presentDepInfo.imageMemoryBarrierCount =
      static_cast<uint32_t>(finalBarriers.size());
  presentDepInfo.pImageMemoryBarriers = finalBarriers.data();
  vkCmdPipelineBarrier2(cmd, &presentDepInfo);

}

void FrameManager::recreateSwapchain() {
  swapchain->recreateSwapChain();
  imageManager.recreateSwapchain();
}

// In the far future: Create pipelines by PipelineKey

void FrameManager::setupSkypassPipeline() {
  PipelineBuilder builder;
  builder.setDefaults(&pipelineManager, PipelineType::Skybox);
  builder.setPipelineDebugLabel("Skypass Pipeline");
  builder.addVertShader("assets/shaders/skyVert.spv");
  builder.addFragShader("assets/shaders/skyFrag.spv");
  builder.disableCulling();
  builder.disableDepthTest();
  builder.disableDepthWrite();
  builder.enablePushConstants(sizeof(SkyPassPushConstant), VK_SHADER_STAGE_FRAGMENT_BIT);
  builder.setAttachmentFormats(1, &swapchain->imageFormat);
  builder.setNoVertexInput();
  builder.build();
}

void FrameManager::setupShadowpassPipeline() {
  PipelineBuilder builder;
  builder.setDefaults(&pipelineManager, PipelineType::Depth);
  builder.setPipelineDebugLabel("Depth Pipeline");
  builder.addVertShader("assets/shaders/shadowVert.spv");
  builder.enablePushConstants(sizeof(MeshPushConstant), VK_SHADER_STAGE_VERTEX_BIT);
  builder.addDescriptor(descriptorAllocator.layouts.camera);
  builder.setVertexFormat(Vertex::getBindingDescription(), Vertex::getAttributeDescriptions());
  builder.enableDepthWrite();
  builder.setAttachmentFormats(0, nullptr, VK_FORMAT_UNDEFINED, VulkanHelper::findDepthFormat(context->physicalDevice));
  builder.setSampleCount(VK_SAMPLE_COUNT_1_BIT);
  builder.build();
}

void FrameManager::setupShadowDescriptor(ImageHandle shadowMap) {
  VkSamplerCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  info.magFilter = VK_FILTER_LINEAR;
  info.minFilter = VK_FILTER_LINEAR;
  info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

  info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;  // No shadow outside of map

  info.anisotropyEnable = VK_FALSE;
  info.compareEnable = VK_TRUE;
  info.compareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
  info.minLod = 0.0f;
  info.maxLod = 0.0f;
  vkCreateSampler(context->device, &info, nullptr, &shadowSampler);

  descriptorAllocator.allocate(descriptorAllocator.layouts.shadows, &shadowDescriptor, 1, "Shadow descriptor");
  
  VkDescriptorImageInfo imageInfo{};
  imageInfo.sampler = shadowSampler;
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  imageInfo.imageView = imageManager.getView(shadowMap);

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstSet = shadowDescriptor;
  write.dstBinding = 0;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.pImageInfo = &imageInfo;

  vkUpdateDescriptorSets(context->device, 1, &write, 0, nullptr);
}

void FrameManager::setupRenderpasses() {
  ImageHandle color = imageManager.allocate(ImageUsageType::ColorImage, "Color image");
  ImageHandle depth = imageManager.allocate(ImageUsageType::DepthImage, "Depth image");
  shadow = imageManager.allocate(ImageUsageType::ShadowMap, "Shadow map");
  setupShadowDescriptor(shadow);
  setupSkypassPipeline();
  setupShadowpassPipeline();
  skyPass.setAttachments(color);
  shadowPass.setAttachments(shadow);
  meshPass.setAttachments(color, depth);
}

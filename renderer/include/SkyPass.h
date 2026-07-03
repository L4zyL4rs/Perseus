#pragma once
#include "IRenderPass.h"
#include "ImageManager.h"
#include "PipelineManager.h"
#include <iterator>
#include <vulkan/vulkan_core.h>

class SkyPass : public IRenderPass {
public:
  ImageManager& manager;
  ImageHandle colorHandle;
  DrawItem item;

  SkyPass(ImageManager& imageManager) : manager(imageManager) {}

  void setAttachments(ImageHandle color) {
    colorHandle = color;
  }

  void addDrawItem(const DrawItem& skyInfo) override { item = skyInfo; }

  void prepare() override {}

  VkImage getOutputImage() const override {
    return manager.getImg(colorHandle);
  }

  void execute(VkCommandBuffer cmd, PipelineManager& pipelineManager) override {
    beginRendering(cmd);
    setViewportScissors(cmd);
    //drawDrawItems(cmd);
    vkCmdEndRendering(cmd);
  }

  void beginRendering(VkCommandBuffer cmd) {
    manager.sync(cmd, colorHandle,
                 VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                 VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

    VkRenderingAttachmentInfo colorAttachmentInfo{};
    colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachmentInfo.imageView = manager.getView(colorHandle);
    colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentInfo.clearValue.color = {0.2f, 0.2f, 0.8f};

    VkRenderingInfoKHR renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    renderInfo.renderArea.offset = {0, 0};
    renderInfo.renderArea.extent = manager.swapExtent;
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &colorAttachmentInfo;

    vkCmdBeginRendering(cmd, &renderInfo);
  }

  void setViewportScissors(VkCommandBuffer cmd) {
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(manager.swapExtent.width);
    viewport.height = static_cast<float>(manager.swapExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = manager.swapExtent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);
  }
};

  

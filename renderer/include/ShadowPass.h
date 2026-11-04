#include "IRenderPass.h"
#include <vulkan/vulkan_core.h>

class ShadowPass : public IRenderPass {
public:
  ImageManager &manager;
  ImageHandle shadowMap;
  std::vector<DrawItem> items;

  ShadowPass(ImageManager &imageManager)
      : manager(imageManager) {}

  ~ShadowPass() override = default;

  void setAttachments(ImageHandle shadowMap) { this->shadowMap = shadowMap; }

  void setDrawItems(std::vector<DrawItem> newItems) override {
    items = newItems;
  }

  void addDrawItem(DrawItem item) {
    items.emplace_back(item);
  }

  void clearDrawItems() {
    items.clear();
  }

  void prepare() override {}

  VkImage getOutputImage() const override { return manager.getImg(shadowMap); }

  void execute(VkCommandBuffer cmd, PipelineManager &pipelineManager) override {
    beginRendering(cmd);
    setViewportScissor(cmd);
    for (auto &item : items) {
      drawDrawItem(cmd, item, pipelineManager);
    }
    vkCmdEndRendering(cmd);
  }

  void beginRendering(VkCommandBuffer cmd) {
    manager.sync(cmd, shadowMap,
                 VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                     VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                 VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                     VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT_KHR);
    VkRenderingAttachmentInfo shadowAttachmentInfo{};
    shadowAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    shadowAttachmentInfo.imageView = manager.getView(shadowMap);
    shadowAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    shadowAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    shadowAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    shadowAttachmentInfo.clearValue = {1.0, 0.0};

    VkRenderingInfo renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderInfo.renderArea.offset = {0, 0};
    renderInfo.renderArea.extent = manager.getExtent(shadowMap);
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 0;
    renderInfo.pColorAttachments = nullptr;
    renderInfo.pDepthAttachment = &shadowAttachmentInfo;

    vkCmdBeginRendering(cmd, &renderInfo);
  }

  void setViewportScissor(VkCommandBuffer cmd) {
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(manager.getExtent(shadowMap).width);
    viewport.height = static_cast<float>(manager.getExtent(shadowMap).height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = manager.getExtent(shadowMap);
    vkCmdSetScissor(cmd, 0, 1, &scissor);
  }

};

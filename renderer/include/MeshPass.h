#include "IRenderPass.h"
#include "ImageManager.h"
#include <vector>
#include <vulkan/vulkan_core.h>

// Dynamic rendering requires explicit syncing, also at begin of draw
// What handles this?
// Idea: Transition from undefined->first draw is handled by image manager
// Maybe one can also stack "syncing" actions by specifying the next pipeline stage?
// Something like: manager.sync(handle, VK_PIPELINE_STAGE_COLOR...)
// Then it will automatically generate and write the required barrier
// Maybe read this next: https://www.sctheblog.com/blog/vulkan-synchronization/
//
// From the article:
// vkCmdDraw(). Used to render triangles to attachments. Usually does something in stages for *_VERTEX_SHADER_BIT and *_FRAGMENT_SHADER_BIT. Depth and stencil operations (both tests and writes) are in stages for *_EARLY_FRAGMENT_TESTS_BIT and *_LATE_FRAGMENT_TESTS_BIT. We have already seen

class MeshPass : public IRenderPass {
public:
    ImageManager& manager;
    ImageHandle colorHandle;
    ImageHandle depthHandle;
    std::vector<DrawItem> items;

    MeshPass(ImageManager& imageManager) : manager(imageManager) {
        colorHandle = manager.allocate(ImageUsageType::ColorImage, "Color image");
        depthHandle = manager.allocate(ImageUsageType::DepthImage, "Depth Image");
    }

    ~MeshPass() override {
        manager.free(colorHandle);
        manager.free(depthHandle);
    }
    
    void addDrawItem(const DrawItem &item) override {
        items.emplace_back(item);
    }

    void prepare() override {
        items.clear();
    }

    VkImage getOutputImage() const override {
        return manager.getImg(colorHandle);
    }

    void execute(VkCommandBuffer cmd, PipelineManager& pipelineManager) override {
        beginRendering(cmd);
        setViewportScissor(cmd);
        drawDrawItems(cmd, items, pipelineManager);
        vkCmdEndRendering(cmd);
    }

    void beginRendering(VkCommandBuffer cmd) {
        manager.sync(cmd, 
                     colorHandle, 
                     VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, 
                     VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
        manager.sync(cmd, 
                     depthHandle, 
                     VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, 
                     VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT);

        std::cout << "Using extent with " << manager.swapExtent.height << " x " << manager.swapExtent.width << "\n";

        VkRenderingAttachmentInfo colorAttachmentInfo{};
        colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachmentInfo.imageView = manager.getView(colorHandle);
        colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentInfo.clearValue.color = { {0.0f, 0.0f, 0.0f, 0.0f} };

        VkRenderingAttachmentInfo depthStencilAttachmentInfo {};
        depthStencilAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthStencilAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthStencilAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthStencilAttachmentInfo.imageView = manager.getView(depthHandle);
        depthStencilAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        depthStencilAttachmentInfo.clearValue = {1.0, 0.0};

        VkRenderingInfoKHR renderInfo {};
        renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
        renderInfo.renderArea.offset = {0, 0};
        renderInfo.renderArea.extent = manager.swapExtent;
        renderInfo.layerCount = 1;
        renderInfo.colorAttachmentCount = 1;
        renderInfo.pColorAttachments = &colorAttachmentInfo;
        renderInfo.pDepthAttachment = &depthStencilAttachmentInfo;

        vkCmdBeginRendering(cmd, &renderInfo);


    }

    void setViewportScissor(VkCommandBuffer cmd) {
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(manager.swapExtent.width);
        viewport.height = static_cast<float>(manager.swapExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = manager.swapExtent;
        vkCmdSetScissor(cmd, 0, 1, &scissor);
    }
};


#include "DrawItemAssembler.h"
#include "ImageManager.h"
#include <vulkan/vulkan_core.h>

class IRenderPass {
public:
    virtual ~IRenderPass() = default;

    virtual void addDrawItem(const DrawItem& item) = 0;

    virtual void prepare() = 0;

    virtual VkImage getOutputImage() const { return nullptr; }

    virtual void execute(VkCommandBuffer cmd, PipelineManager& pipelineManager) = 0;

    inline void drawDrawItems(VkCommandBuffer cmd, const std::vector<DrawItem>& items, PipelineManager& pipelineManager) {
        for(auto& item : items) {
            VkDeviceSize offsets[] = {0};
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineManager.pipeline(item.pipeline));
            vkCmdBindVertexBuffers(cmd, 0, 1, &item.meshBuffer, offsets);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineManager.getPipelineLayout(item.pipeline), 0, item.descriptorSets.size(), item.descriptorSets.data(), 0, nullptr);
            MeshPushConstant pushConstant{item.transform};
            if(pipelineManager.pushConstantSizes[item.pipeline] != 0) {
                vkCmdPushConstants(cmd, pipelineManager.getPipelineLayout(item.pipeline), VK_SHADER_STAGE_VERTEX_BIT, 0, pipelineManager.pushConstantSizes[item.pipeline], &pushConstant);
            }
            if(item.indexBuffer == nullptr) {
                vkCmdDraw(cmd, item.meshStopIndex - item.meshStartIndex + 1, 1, item.meshStartIndex, 0);
            }
            else {
                vkCmdBindIndexBuffer(cmd, item.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(cmd, item.meshStopIndex - item.meshStartIndex + 1, 1, item.meshStartIndex, 0, 0);
            }
        }
    }
};


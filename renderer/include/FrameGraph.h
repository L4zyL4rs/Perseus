#include <vector>
#include <stdint.h>
#include "DrawItemAssembler.h"
#include <vulkan/vulkan_core.h>

using ImageHandle = uint32_t;
using PassHandle = uint32_t;    // Not sure if needed

struct ImageDescriptor {
    VkImageView* pExternalImage = nullptr;  // For swapchain image
    VkExtent2D extent;
    VkFormat format;
    VkImageUsageFlags usage;    // For now supprt only color attachment
                                // and depth stencil bit
};

// Nonhazards are:
// ??Write after read
// Read after read
enum class FrameGraphUsageFlag {
    NoUsage,
    ColorWrite,
    ColorRead,
    //ColorReadWrite,
    DepthWrite,
    //DepthRead,
    DepthReadWrite
};

struct ImageBarrierPrototype {
   VkImageLayout oldLayout;
   VkImageLayout newLayout;
   ImageHandle image;
   VkImageAspectFlagBits aspect;
};
   
struct ImageNode {
    ImageHandle image;
    VkImageLayout layout;
    VkPipelineStageFlags2 stageMask; 
    VkImageAspectFlagBits aspect;
};

// Will hold usage and image descriptors
struct PassInfo {
    uint32_t attachmentCount;
    ImageHandle* pHandles;
    FrameGraphUsageFlag* pUsages;
    std::function<void(VkCommandBuffer, std::vector<DrawItem>&)> execute;
};

struct FrameGraph {
    FrameGraph(std::vector<std::vector<PassInfo>> passes, std::vector<std::vector<ImageBarrierPrototype>> barriers);
    std::vector<std::vector<ImageBarrierPrototype>> barriers;
    std::vector<std::vector<std::function<void(VkCommandBuffer, std::vector<DrawItem>&)>>> graph;
};


// Idea for now:
// Store each pass after another, as they are submitted
// Then see if one can squish neighbouring passes next to another
class FrameGraphBuilder {
public:
    void submitPass(PassInfo info);
    FrameGraph build();
private:
    std::vector<FrameGraphUsageFlag> getUsages(PassInfo& pass);
    std::vector<PassInfo> passes;
    std::vector<ImageBarrierPrototype> createBarriers(std::vector<ImageNode>& old, std::vector<ImageNode>& next);
    bool tryMerge(std::vector<PassInfo>& dst, PassInfo& src);
    size_t maxImageHandle = 0;
};


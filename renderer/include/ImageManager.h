#include "RenderContext.h"
#include "VulkanHelper.h"
#include <vector>
#include "FrameGraph.h"
#include <vulkan/vulkan_core.h>



struct ImageWrapper {
    ImageDescriptor desc;
    VkImage image;
    VkImageView view;
    VmaAllocation allocation;
};

// Manager takes in descriptors and returns handles to memory
// In the future: Allow for aliasing, different handles might return
// images that live in the same memory region.
class ImageManager {
    std::vector<ImageWrapper> images;
public:
    ImageHandle create(ImageDescriptor desc);
    void submit(FrameGraph graph, const RenderContext& context);
};


#include "ImageManager.h"
#include "RenderContext.h"
#include "VulkanHelper.h"
#include <vulkan/vulkan_core.h>

ImageHandle ImageManager::create(ImageDescriptor desc) {
    images.emplace_back(desc);
    return images.size() -1;
}

// FrameGraph is passed to do aliasing, do not worry about it for now
void ImageManager::submit(FrameGraph graph, const RenderContext& context) {
    for(auto& img : images) {
        VkDeviceMemory memory;
        VulkanHelper::createImage(
            &context,
            img.desc.extent.width,
            img.desc.extent.height,
            1,
            context.msaaSamples,
            img.desc.format,
            VK_IMAGE_TILING_OPTIMAL,
            img.desc.usage,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            img.image,
            memory,
            &img.allocation
        );
        VulkanHelper::createImageView(
            &context,
            img.image,
            img.desc.format,
            img.
    }
}

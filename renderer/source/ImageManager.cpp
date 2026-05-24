#include "ImageManager.h"
#include "RenderContext.h"
#include "glm/vector_relational.hpp"
#include <ranges>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

ImageManager::ImageManager(RenderContext& context, VkExtent2D& swapExtent, VkFormat& swapFormat) :
    context(context),
    swapExtent(swapExtent),
    swapFormat(swapFormat){}

ImageManager::~ImageManager() {
    for(auto& image : images) {
        vkDestroyImageView(context.device, image.view, nullptr);
        vmaDestroyImage(context.vmaAllocator, image.image, image.allocation);
    }
}



// Try to infer format, msaa samples etc from render context
ImageHandle ImageManager::allocate(ImageUsageType usage, std::string debugLabel) {
    VkImageUsageFlags usageBits;
    VkExtent2D extent = swapExtent;
    VkFormat format = swapFormat;
    VkSampleCountFlagBits msaaSamples = context.msaaSamples;
    VkImageAspectFlags aspectMask;
    uint32_t mipLevels = 1;

    switch(usage) {
        case ImageUsageType::ColorImage:
            usageBits = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            break;
        case ImageUsageType::DepthImage:
            format = VulkanHelper::findDepthFormat(context.physicalDevice);
            usageBits = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            break;
        default:
            throw std::logic_error("Cannot automatically allocate this image type");
    }       
    
    ImageHandle handle = allocate(usageBits, extent, format, msaaSamples, aspectMask, mipLevels, debugLabel);
    images[handle].extentIsSwapExtent = true;
    return handle;
}

ImageHandle ImageManager::allocate(VkImageUsageFlags usage, VkExtent2D extent, VkFormat format, VkSampleCountFlagBits msaaSamples, VkImageAspectFlags aspectMask, uint32_t mipLevels, std::string debugLabel) {
    if(mipLevels == 0) {
        throw std::runtime_error("MipLevels must be greater than 0");
    }

    Image image;
    image.handle = images.size();

    std::cout << "Using extent with " << extent.height << " x " << extent.width << "\n";

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.height = extent.height;
    imageInfo.extent.width = extent.width;
    imageInfo.extent.depth = 1;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.flags = 0;
    imageInfo.arrayLayers = 1;
    imageInfo.usage = usage;
    imageInfo.samples = msaaSamples;
    imageInfo.format = format;
    imageInfo.mipLevels = mipLevels;

    VmaAllocationCreateInfo allocInfo {};
    allocInfo.flags = VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    if(vmaCreateImage(context.vmaAllocator, &imageInfo, &allocInfo, &image.image, &image.allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image!");
    }
    std::cout << "Created image " << debugLabel << "\n";

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;

    image.subresource.baseArrayLayer = 0;
    image.subresource.baseMipLevel = 0;
    image.subresource.layerCount = 1;
    image.subresource.aspectMask = aspectMask;
    image.subresource.levelCount = mipLevels;

    viewInfo.subresourceRange = image.subresource;

    if(vkCreateImageView(context.device, &viewInfo, nullptr, &image.view) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image view!");
    }

    context.setDebugLabel(VK_OBJECT_TYPE_IMAGE, image.image, debugLabel);

    image.valid = true;

    ImageAllocInfo imageAllocInfo;
    imageAllocInfo.usage = usage;
    imageAllocInfo.extent = extent;
    imageAllocInfo.format = format;
    imageAllocInfo.msaaSamples = msaaSamples;
    imageAllocInfo.aspectMask = aspectMask;
    imageAllocInfo.mipLevels = mipLevels;
    imageAllocInfo.debugLabel = debugLabel;

    image.allocInfo = imageAllocInfo;

    images.emplace_back(image);

    return image.handle;
}

ImageHandle ImageManager::allocate(ImageAllocInfo allocInfo) {
    return allocate(allocInfo.usage,
                    allocInfo.extent,
                    allocInfo.format,
                    allocInfo.msaaSamples,
                    allocInfo.aspectMask,
                    allocInfo.mipLevels,
                    allocInfo.debugLabel);
}

void ImageManager::free(ImageHandle handle) {
    free(images[handle]);
}

void ImageManager::free(Image image) {
    vkDestroyImageView(context.device, image.view, nullptr);
    vmaDestroyImage(context.vmaAllocator, image.image, image.allocation);
    image.valid = false;
}

VkImage ImageManager::getImg(ImageHandle handle) {
    return images[handle].image;
}

VkImageView ImageManager::getView(ImageHandle handle) {
    return images[handle].view;
}

void ImageManager::sync(VkCommandBuffer commandBuffer, ImageHandle handle, VkPipelineStageFlags2 stage, VkAccessFlags2 access) {
    VkImageMemoryBarrier2 barrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = images[handle].sync.stage,
        .srcAccessMask = images[handle].sync.access,
        .dstStageMask = stage,
        .dstAccessMask = access,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .image = images[handle].image,
        .subresourceRange = images[handle].subresource
    };
    
    VkDependencyInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    info.imageMemoryBarrierCount = 1;
    info.pImageMemoryBarriers = &barrier;
    // Dependency flags irrelevant?
    
    vkCmdPipelineBarrier2(commandBuffer, &info);

    images[handle].sync.stage = stage;
    images[handle].sync.access = access;
}

void ImageManager::recreateSwapchain() {
    for(size_t i = 0; i < images.size(); i++) {
        if(images[i].extentIsSwapExtent) {
            recreateImage(i);
        }
    }
}

void ImageManager::recreateImage(ImageHandle handle) {
    ImageAllocInfo allocInfo = images[handle].allocInfo;
    allocInfo.extent = swapExtent;
    std::cout << "Fortnite " << allocInfo.extent.width << "\n";

    free(handle);
    ImageHandle tempHandle = allocate(allocInfo);
    images[handle] = images[tempHandle];
    images.pop_back();
}
    

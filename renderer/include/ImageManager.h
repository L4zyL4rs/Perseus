#pragma once
#include "RenderContext.h"
#include "VulkanHelper.h"
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

using ImageHandle = uint32_t;

struct ImageSyncInfo {
    VkPipelineStageFlags2 stage;
    VkAccessFlags2 access;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
};

struct ImageAllocInfo {
    VkImageUsageFlags usage{};
    VkExtent2D extent{};
    VkFormat format{};
    VkSampleCountFlagBits msaaSamples{};
    VkImageAspectFlags aspectMask{};
    uint32_t mipLevels{};
    std::string debugLabel{};
};

enum class ImageUsageType {
    ColorImage,
    DepthImage,
    Sampled
};

struct Image {
    ImageHandle handle{};
    VkImage image{};
    VmaAllocation allocation{}; // Allocation acts something like a pointer one gets for malloc/free
    VkImageView view{};
    ImageSyncInfo sync{};
    VkImageSubresourceRange subresource{};
    ImageAllocInfo allocInfo{};
    bool extentIsSwapExtent{};
    bool valid{};
};


class ImageManager{
public:
    RenderContext& context;
    VkExtent2D& swapExtent;
    VkFormat& swapFormat;
    ImageManager(RenderContext& context, VkExtent2D& swapExtent, VkFormat& swapFormat);
    ~ImageManager();
    ImageHandle allocate(ImageUsageType usage, std::string debugLabel = "Generic image");
    ImageHandle allocate(VkImageUsageFlags usage, VkExtent2D extent, VkFormat format, VkSampleCountFlagBits msaaSamples, VkImageAspectFlags aspectmask, uint32_t mipLevels, std::string debugLabel = "Generic Image");
    ImageHandle allocate(ImageAllocInfo info);
    void free(ImageHandle handle);
    void free(Image image);
    VkImage getImg(ImageHandle handle);
    VkImageView getView(ImageHandle handle);
    void sync(VkCommandBuffer commandBuffer, ImageHandle handle, VkPipelineStageFlags2 stage, VkAccessFlags2 access);   // Make this easier to interface in the future:wa
    void recreateSwapchain();
    void recreateImage(ImageHandle handle);

private:
    std::vector<Image> images{};
};

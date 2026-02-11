#pragma once
#include <VulkanHelper.h>
#include <algorithm>

class Swapchain {
public:
    RenderContext* context;
    AppWindow* window;
    VkSwapchainKHR swapChain{};
    std::vector<VkImage> images{};
    std::vector<VkFramebuffer> framebuffers{};
    VkFormat imageFormat{};
    std::vector<VkImageView> imageViews{};
    VmaAllocation imageAllocation{};
    VkExtent2D extent{};

    VkImage colorImage{};
    VkDeviceMemory colorImageMemory{};
    VkImageView colorImageView{};

    VkImage depthImage{};
    VkDeviceMemory depthImageMemory{};
    VkImageView depthImageView{};
    VmaAllocation depthImageAllocation{};
    VkRenderPass renderPass{};

    Swapchain(RenderContext* c);
    void createSwapChain();
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void recreateSwapChain();
    void cleanupSwapChain();
    void createFramebuffers();
    void createImageViews();
    void createColorResources();
    void createDepthResources();
    void createRenderPass();
    VkFormat findDepthFormat();
    void cleanup();
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
};
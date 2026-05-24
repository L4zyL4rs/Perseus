#pragma once
#include <VulkanHelper.h>
#include "ImageManager.h"
#include <algorithm>

class Swapchain {
public:
    RenderContext* context;
    AppWindow* window;
    ImageManager imageManager;
    VkSwapchainKHR swapChain{};
    std::vector<VkImage> images{};
    std::vector<VkImageView> imageViews{};
    std::vector<VkFramebuffer> framebuffers{};
    VkFormat imageFormat{};
    VmaAllocation imageAllocation{};
    VkExtent2D extent{};

    //ImageHandle colorImage{};
    //ImageHandle depthImage{};
    
    VkRenderPass renderPass{};  // Deprecated

    Swapchain(RenderContext* c);
    void createSwapChain();
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void recreateSwapChain();
    void cleanupSwapChain();
    void createImageViews();
    void createColorResources();
    void createDepthResources();
    void createRenderPass();
    void cleanup();
};


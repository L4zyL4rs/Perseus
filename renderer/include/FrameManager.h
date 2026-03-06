#pragma once
#include "renderObject.h"
#include "FontRenderer.h"

extern uint32_t MAX_OBJECTS;

class FrameManager {
public:
    Swapchain* swapchain{};
    RenderContext* context{};
    AppWindow* window{};
    PipelineManager pipelineManager;
    CommandPool commandPool;
    uint32_t MAX_FRAMES_IN_FLIGHT;
    DescriptorAllocator descriptorAllocator;
    AssetManager assetManager;
    ObjectManager objectManager;
    FontRenderer fontRenderer;
    std::vector<VkCommandBuffer> commandBuffers{};
    std::vector<VkSemaphore> imageAvailableSemaphores{};
    std::vector<VkSemaphore> renderFinishedSemaphores{};
    std::vector<VkFence> inFlightFences{};
    std::thread physicsThread{};
    uint32_t currentFrame = 0;
    std::atomic<bool> MAIN_LOOP_RUNNING = false;

    FrameManager(Swapchain* s);
    void cleanup();
    void createCommandBuffers();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, std::vector<DrawItem>& drawItems);
    void draw(std::vector<DrawItem>& drawItems);
    void createSyncObjects();
    // Insane naming
    void drawDrawItems(const std::vector<DrawItem>& items, VkCommandBuffer commandBuffer, uint32_t currentFrame);
private:
    void transitionLayoutPresent(uint32_t imageIndex);
    void transitionLayoutRender(uint32_t imageIndex);
};

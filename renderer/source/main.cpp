//#pragma once
////#include "Context.h"
//#include <iostream>
//#include <cstdlib>
//#include <cstring>
//#include "FrameManager.h"
//#include "SceneLoader.h"
//
//// Wrapper functions for aligned memory allocation
//// There is currently no standard for this in C++ that works across all platforms and vendors, so we abstract this
//// Stolen from https://github.com/SaschaWillems/Vulkan/blob/master/examples/dynamicuniformbuffer/dynamicuniformbuffer.cpp
////void* alignedAlloc(size_t size, size_t alignment)
////{
////    void* data = nullptr;
////#if defined(_MSC_VER) || defined(__MINGW32__)
////    data = _aligned_malloc(size, alignment);
////#else
////    int res = posix_memalign(&data, alignment, size);
////    if (res != 0)
////        data = nullptr;
////#endif
////    return data;
////}
////
////void alignedFree(void* data)
////{
////#if	defined(_MSC_VER) || defined(__MINGW32__)
////    _aligned_free(data);
////#else
////    free(data);
////#endif
////}
//
//class HelloTriangleApplication {
//public:
//    HelloTriangleApplication() : window(), context(&window), swapchain(&context), frameManager(&swapchain) {}
//    void run() {
//        mainLoop();
//        cleanup();
//    }
//
//private:
//    AppWindow window;
//    RenderContext context;
//    Swapchain swapchain;
//    FrameManager frameManager;
//
//    std::thread physicsThread;
//
//    void mainLoop() {
//        while (!window.windowShouldClose()) {
//            window.tick();
//            frameManager.draw();
//        }
//
//        vkDeviceWaitIdle(context.device);
//    }
//
//    void cleanup() {
//        frameManager.cleanup();
//        swapchain.cleanup();
//        context.cleanup();
//        window.destroyWindow();
//
//        glfwTerminate();
//    }
//
//
//    // ??? Why
//    bool hasStencilComponent(VkFormat format) {
//        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
//    }
//
//    // ??? Why Why
//    size_t getMinUniformBufferOffsetAlignment() {
//        VkPhysicalDeviceProperties physicalDeviceProperties;
//        vkGetPhysicalDeviceProperties(context.physicalDevice, &physicalDeviceProperties);
//        return physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
//    }
//};
//
//int main() {
//    HelloTriangleApplication app;
//
//    /*_CrtSetDbgFlag(
//        _CRTDBG_ALLOC_MEM_DF |
//        _CRTDBG_LEAK_CHECK_DF |
//        _CRTDBG_CHECK_ALWAYS_DF
//    );*/
//
//    try {
//        app.run();
//    }
//    catch (const std::exception& e) {
//        std::cerr << e.what() << std::endl;
//        return EXIT_FAILURE;
//    }
//
//    return EXIT_SUCCESS;
//}
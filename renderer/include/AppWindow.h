#pragma once
//#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Config.h"
#include "Camera.h"

extern uint32_t WINDOWWITH_INIT;
extern uint32_t WINDOWHEIGHT_INIT;
extern float MOUSESENSE;

void framebufferResizeCallback(GLFWwindow* window, int width, int height);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);

class AppWindow {
public:
    GLFWwindow* window = nullptr;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    uint32_t WINDOWWIDTH = WINDOWWITH_INIT;
    uint32_t WINDOWHEIGHT = WINDOWHEIGHT_INIT;
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    bool framebufferResized = false;    // Keep this here for now, disentangle later
    Camera cam;

    AppWindow();
    void tick();
    void destroySurface(VkInstance& instance);
    void destroyWindow();
    void getFramebufferSize(int* width, int* height);
    const VkSurfaceKHR& getSurface();
    void createSurface(VkInstance instance);
    bool windowShouldClose();
    void processInput();
};

void framebufferResizeCallback(GLFWwindow* window, int width, int height);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);

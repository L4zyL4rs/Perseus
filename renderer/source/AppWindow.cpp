#include "AppWindow.h"
#include "GLFW/glfw3.h"
#include <iostream>

AppWindow::AppWindow(uint32_t WIDTH, uint32_t HEIGHT) :
    WINDOWWIDTH(WIDTH),
    WINDOWHEIGHT(HEIGHT) {
	//std::cout << "Cam size: " << sizeof(Camera) << "\n";
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(WINDOWWIDTH, WINDOWHEIGHT, "Vulkan", nullptr, nullptr);
    // Window manager might have other idea what size the window should be
    int realH, realW;
    glfwGetWindowSize(window, &realW, &realH);
    WINDOWWIDTH = realW;
    WINDOWHEIGHT = realH;
    std::cout << "Window is actually " << WINDOWWIDTH << "x" << WINDOWHEIGHT << " big\n";
	std::cout << "Window pointer: " << this << "\n";
	glfwSetWindowUserPointer(window, this);
	std::cout << "User pointer set to: " << glfwGetWindowUserPointer(window) << "\n";
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	//glfwSetCursorPosCallback(window, mouseCallback);
}

void AppWindow::tick()
{
	float currentFrame = glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	//processInput();

	glfwPollEvents();
}

void AppWindow::destroySurface(VkInstance& instance)
{
	vkDestroySurfaceKHR(instance, surface, nullptr);
}

void AppWindow::destroyWindow()
{
	glfwDestroyWindow(window);
}

void AppWindow::getFramebufferSize(int* width, int* height)
{
	glfwGetFramebufferSize(window, width, height);
}

const VkSurfaceKHR& AppWindow::getSurface()
{
	return surface;
}

void AppWindow::createSurface(VkInstance instance)
{
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

bool AppWindow::windowShouldClose()
{
	return glfwWindowShouldClose(window);
}

void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto app = reinterpret_cast<AppWindow*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}


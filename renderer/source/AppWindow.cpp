#include "AppWindow.h"

AppWindow::AppWindow()
{
	std::cout << "Cam size: " << sizeof(Camera) << "\n";
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(WINDOWWIDTH, WINDOWHEIGHT, "Vulkan", nullptr, nullptr);
	std::cout << "Window pointer: " << this << "\n";
	glfwSetWindowUserPointer(window, this);
	std::cout << "User pointer set to: " << glfwGetWindowUserPointer(window) << "\n";
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, mouseCallback);
}

void AppWindow::tick()
{
	float currentFrame = glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	processInput();

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

void AppWindow::processInput()
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	const float cameraSpeed = 1.5f * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		cam.worldCameraPos.x += cameraSpeed * cam.cameraFront[0];
		cam.worldCameraPos.y += cameraSpeed * cam.cameraFront[1];
		cam.worldCameraPos.z += cameraSpeed * cam.cameraFront[2];
		std::cout << cam.worldCameraPos.x;
		std::cout << "\n";
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		cam.worldCameraPos.x -= cameraSpeed * cam.cameraFront[0];
		cam.worldCameraPos.y -= cameraSpeed * cam.cameraFront[1];
		cam.worldCameraPos.z -= cameraSpeed * cam.cameraFront[2];
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		cam.worldCameraPos.x -= glm::normalize(glm::cross(cam.cameraFront, cam.cameraUp))[0] * cameraSpeed;
		cam.worldCameraPos.y -= glm::normalize(glm::cross(cam.cameraFront, cam.cameraUp))[1] * cameraSpeed;
		cam.worldCameraPos.z -= glm::normalize(glm::cross(cam.cameraFront, cam.cameraUp))[2] * cameraSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		cam.worldCameraPos.x += glm::normalize(glm::cross(cam.cameraFront, cam.cameraUp))[0] * cameraSpeed;
		cam.worldCameraPos.y += glm::normalize(glm::cross(cam.cameraFront, cam.cameraUp))[1] * cameraSpeed;
		cam.worldCameraPos.z += glm::normalize(glm::cross(cam.cameraFront, cam.cameraUp))[2] * cameraSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
		cam.worldCameraPos.x += cameraSpeed * cam.cameraUp[0];
		cam.worldCameraPos.y += cameraSpeed * cam.cameraUp[1];
		cam.worldCameraPos.z += cameraSpeed * cam.cameraUp[2];
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		cam.worldCameraPos.x -= cameraSpeed * cam.cameraUp[0];
		cam.worldCameraPos.y -= cameraSpeed * cam.cameraUp[1];
		cam.worldCameraPos.z -= cameraSpeed * cam.cameraUp[2];
	}
}

void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto app = reinterpret_cast<AppWindow*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
	auto app = reinterpret_cast<AppWindow*>(glfwGetWindowUserPointer(window));
	auto& cam = app->cam;

	float xoffset = xpos - cam.lastX;
	float yoffset = cam.lastY - ypos;
	cam.lastX = xpos;
	cam.lastY = ypos;

	xoffset *= MOUSESENSE;
	yoffset *= MOUSESENSE;

	// Pitch and yaw calculation OK
	cam.yaw += xoffset;
	cam.pitch += yoffset;

	if (cam.pitch > 89.0f)
		cam.pitch = 89.0f;
	if (cam.pitch < -89.0f)
		cam.pitch = -89.0f;

	glm::vec3 direction;
	direction.x = cos(glm::radians(-cam.yaw)) * cos(glm::radians(cam.pitch));
	direction.y = sin(glm::radians(-cam.yaw)) * cos(glm::radians(cam.pitch));
	direction.z = sin(glm::radians(cam.pitch));


	cam.cameraFront = glm::normalize(direction);
	std::cout << "Address in mouseCallback: " << &cam.cameraFront << "\n";
	std::cout << "Size of window: " << sizeof(Camera) << "\n";
}

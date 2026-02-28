#pragma once
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "AppWindow.h"
#include "vma.h"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct DebugUtils {
    PFN_vkSetDebugUtilsObjectNameEXT vkSetObjectName = nullptr;
    PFN_vkQueueBeginDebugUtilsLabelEXT vkQueueBeginLabel = nullptr;
    PFN_vkQueueEndDebugUtilsLabelEXT vkQueueEndLabel = nullptr;
    PFN_vkQueueInsertDebugUtilsLabelEXT vkQueueInsertLabel = nullptr;
    PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginLabel = nullptr;
    PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndLabel = nullptr;
    PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertLabel = nullptr;
};

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

class RenderContext {
public:
	AppWindow* window;
	VkDevice device;
	VkPhysicalDevice physicalDevice{};
	VmaAllocator vmaAllocator{};
	VkInstance instance{};
	VkDebugUtilsMessengerEXT debugMessenger{};
	VkQueue presentQueue{};
	VkQueue graphicsQueue{};
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> deviceExtensions = {
				VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	RenderContext(AppWindow* w);
	void cleanup();
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	VkQueue getPresentQueue() const;
	VkInstance getInstance() const;

    template <typename T>
    inline void setDebugLabel(VkObjectType type, const T& object, const std::string& label) {
        VkDebugUtilsObjectNameInfoEXT nameInfo {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = type;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(object);
        nameInfo.pObjectName = label.c_str();
        nameInfo.pNext = NULL;

        if(debugUtils.vkSetObjectName(device, &nameInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to set debug label!");
        }
    }

    void QueueBeginLabel(VkQueue queue, const std::string& label);
    void QueueEndLabel(VkQueue queue);
    void QueueInsertLabel(VkQueue queue, const std::string& label);
    void CmdBeginLabel(VkCommandBuffer buffer, const std::string& label);
    void CmdEndLabel(VkCommandBuffer buffer);
    void CmdInsertLabel(VkCommandBuffer buffer, const std::string& label);

private:
    DebugUtils debugUtils{};
	void pickPhysicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice device);
	bool checkDeviceExtensionsSupport(VkPhysicalDevice device);
	void createInstance();
	bool checkValidationLayerSupport();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
	std::vector<const char*> getRequiredExtensions();
	void setupDebugMessenger();
	void createLogicalDevice();
	VkSampleCountFlagBits getMaxUsableSampleCount();
	void createMemoryAllocator();
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
    void createDebugUtils();
    

};

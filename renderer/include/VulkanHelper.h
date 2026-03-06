#pragma once
#include "RenderContext.h"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <iostream>
#include <ft2build.h>
#include <vulkan/vulkan_core.h>
#include "glm_config.h"
#include FT_FREETYPE_H

// Lookup table for using VkFormat and channels of bitmaps interchangeably
// Prefer this over VkFormat wherever possible
struct PixelFormatInfo {
	VkFormat format{};
	uint32_t channels{};
	uint32_t bytesPerPixel{};
};

// Maybe write a comment next time why this is needed
// I remember that it is, but no idea why
struct CommandPool {
public:
	CommandPool(RenderContext* c);
	~CommandPool();
	VkCommandPool get() const;

private:
	RenderContext* context;
	VkCommandPool commandPool;
};

static constexpr PixelFormatInfo pixelFormatTable[]{
	{VK_FORMAT_R8_SRGB,			1, 1},
	{VK_FORMAT_R8G8B8_SRGB,		3, 1},
	{VK_FORMAT_R8G8B8A8_SRGB,   4, 1}
};

// Return format information for VkFormat
const inline PixelFormatInfo* lookupFormat(VkFormat f) {
	for (auto& info : pixelFormatTable) {
		if (info.format == f) return &info;
	}
	return nullptr;
}

// Return format information matching parameters
const inline PixelFormatInfo* lookupParameters(uint32_t channels, uint32_t bytes) {
	for (auto& info : pixelFormatTable) {
		if (info.channels == channels && info.bytesPerPixel == bytes) return &info;
	}
	return nullptr;
}

inline std::ostream& operator<<(std::ostream& out, glm::mat4 in);
inline std::ostream& operator<<(std::ostream& out, glm::vec4 in);
inline std::ostream& operator<<(std::ostream& out, glm::vec3 in);


class VulkanHelper
{
public:
	static void createBuffer(RenderContext* context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* bufferMemory);
	static uint32_t findMemoryType(RenderContext* context, uint32_t typeFilter, VkMemoryPropertyFlags properties);
	static VkCommandBuffer beginSingleTimeCommands(RenderContext* context, VkCommandPool commandPool);
	static void endSingleTimeCommands(RenderContext* context, VkCommandPool commandPool, VkCommandBuffer commandBuffer);
	static void copyBuffer(RenderContext* context, VkCommandPool commandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	static VkImageView createImageView(RenderContext* context, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
	static void createImage(RenderContext* context, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VmaAllocation* pAllocation);
	static void transitionImageLayout(RenderContext* context, VkCommandPool commandPool, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
	static void copyBufferToImage(RenderContext* context, VkCommandPool commandPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	static void generateMipmaps(RenderContext* context, VkCommandPool commandPool, VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
	static std::vector<char> readFile(const std::string& filename);
};


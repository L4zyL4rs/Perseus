#pragma once
#include <cstdlib>
#include <cstdint>
#include <vector>
#include "PipelineBuilder.h"
#include "AssetManager.h"
#include "DrawItemAssembler.h"
#include <fstream>
#include <filesystem>


extern std::string fontPathTerminal;

// This into asset manager
struct FontBundle {
	std::string fontPath;
	int fontSize;
	VkImageView fontAtlas;
	std::unordered_map<char, CharacterInfo> characters;
	std::vector<VkDescriptorSet> atlasDescriptorSet;
	VmaAllocation atlasImageAllocation;
	VkImage atlasImage;
	VkDeviceMemory atlasImageMemory;

	// These should now get an index buffer
	uint32_t firstIndex;
	uint32_t lastIndex;
};

// This into ECS
struct TextElement {
	glm::vec2 screenPosUv;
	std::vector<CharacterCoordinates> text;
	FontHandle handle;
};

struct UpdatedTextElement {
	glm::vec2 screenPosUv;
	std::vector<CharacterCoordinates> text;
	FontHandle handle;
};

class FontRenderer {
public:
	FontRenderer(RenderContext* c, DescriptorAllocator* dA, PipelineManager* pM, AssetManager* aM, CommandPool* cP);
	TextElement createTextElement(std::string fontPath, int fontSize, std::string text, glm::vec2 pos);
	void cleanup();
	const std::vector<DrawItem>& assembleDrawItems(std::unordered_map<FontHandle, std::vector<CharacterCoordinates>>& texts, std::vector<CharacterCoordinates>& textBuffer);
	void draw(VkCommandBuffer commandBuffer, uint32_t imageIndex);
private:
	RenderContext* context;
	DescriptorAllocator* descriptorAllocator;
	PipelineManager* pipelineManager;
	AssetManager* assetManager;
	CommandPool* commandPool;
	VkBuffer stagingBuffer{};
	VkDeviceMemory stagingBufferMemory{};
	void* pStagingBuffer{};
	size_t characterBufferSize = 10000;
	VkBuffer characterBuffer{};
	VkDeviceMemory characterBufferMemory{};
	std::vector<FontHandle> fontHandles{};
	std::vector<TextElement> textElements{};
	std::vector<DrawItem> drawItems{};
	Bitmap loadFontBitmap(const std::string& fontPath, int fontSize, FontBundle* pFontBundle, int padding = 2);
	void createCharacterBuffer();
	void createCharacterCoordinates(std::string text, TextElement* pTextElement);
	void initStagingBuffer(int size);
	void destroyStagingBuffer();
	void destroyCharacterBuffer();
	void createTextGraphicsPipeline();
};
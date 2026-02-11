#include "AssetManager.h"
#include <array>

#ifndef TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#endif

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif

bool Vertex::operator==(const Vertex& other) const
{
	return pos == other.pos && color == other.color && texCoord == other.texCoord;
}

VkVertexInputBindingDescription Vertex::getBindingDescription()
{
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription> Vertex::getAttributeDescriptions()
{
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
	attributeDescriptions.resize(3);
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, pos);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, color);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

	return attributeDescriptions;
}

VkVertexInputBindingDescription CharacterCoordinates::getBindingDescription()
{
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(CharacterCoordinates);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription> CharacterCoordinates::getAttributeDescriptions()
{
	std::vector<VkVertexInputAttributeDescription> attributeDescription;
	attributeDescription.resize(3);

	attributeDescription[0].binding = 0;
	attributeDescription[0].location = 0;
	attributeDescription[0].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescription[0].offset = offsetof(CharacterCoordinates, textureUv);

	attributeDescription[1].binding = 0;
	attributeDescription[1].location = 1;
	attributeDescription[1].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescription[1].offset = offsetof(CharacterCoordinates, screenUv);

	attributeDescription[2].binding = 0;
	attributeDescription[2].location = 2;
	attributeDescription[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attributeDescription[2].offset = offsetof(CharacterCoordinates, color);

	return attributeDescription;
}

AssetManager::AssetManager(RenderContext* c, CommandPool* cP, DescriptorAllocator* dA) : context(c)
, commandPool(cP)
, descriptorAllocator(dA)
{
	createFontSampler();
	std::string cube = "cube";
	loadAsset(cube);
	std::string sphere = "sphere";
	loadAsset(sphere);
}

FontHandle AssetManager::loadFont(std::string& path, size_t fontSize)
{
	if (!fontResources.empty()) {
		for (size_t i = 0; i < fontResources.size(); i++) {
			if (fontResources[i].path == path && fontResources[i].fontSize == fontSize) {
				return i;
			}
		}
	}

	FontHandle handle = fontResources.size();
	FontResource resource;


	resource.path = path;
	resource.fontSize = fontSize;
	Bitmap bitmap = loadFontBitmap(path, fontSize, resource);
	createAtlasImageView(resource, bitmap);
	createAtlasDescriptorSet(resource);
	fontResources.push_back(resource);
	return handle;
}

MeshHandle AssetManager::loadAsset(std::string& name)
{

	if (!meshResources.empty()) {
		for (int i = 0; i < meshResources.size(); i++) {
			if (meshResources[i].name == name) {
				return i;
			}
		}
	}

	// No hot loading models for now :c
	assert(!buffersCreated);

	MeshHandle mesh = meshResources.size();
	TextureHandle texture = textureResources.size();
	MeshResource meshResource;
	Texture textureResource;
	meshResource.name = name;
	meshResource.texture = texture;
	textureResource.name = name;
	meshResources.push_back(meshResource);
	textureResources.push_back(textureResource);

	loadMesh(mesh);
	createTexture(texture);
	createMaterialData(texture);
	createTextureDescriptorSets(texture);

	return meshResources.size() - 1;
}

const FontResource& AssetManager::getFont(FontHandle handle)
{
	//std::cout << fontResources.size();
	return fontResources[handle];
}

const MeshResource& AssetManager::getMesh(MeshHandle handle)
{
	return meshResources[handle];
}

const std::vector<VkDescriptorSet>& AssetManager::getDescriptors(TextureHandle handle) const
{
	return textureResources[handle].descriptors;
}

void AssetManager::createBuffers()
{
	buffersCreated = true;

	createVertexBuffer();
	createIndexBuffer();
}

void AssetManager::cleanup()
{
	for (size_t i = 0; i < meshResources.size(); i++) {
		cleanupObject(i);
	}

	for (size_t i = 0; i < fontResources.size(); i++) {
		cleanupFont(i);
	}

	vkDestroySampler(context->device, fontSampler, nullptr);

	vkDestroyBuffer(context->device, indexBuffer, nullptr);
	vkFreeMemory(context->device, indexBufferMemory, nullptr);

	vkDestroyBuffer(context->device, vertexBuffer, nullptr);
	vkFreeMemory(context->device, vertexBufferMemory, nullptr);
}

const VkBuffer& AssetManager::getIndexBuffer()
{
	return indexBuffer;
}

const VkBuffer& AssetManager::getVertexBuffer()
{
	return vertexBuffer;
}

void AssetManager::loadMesh(MeshHandle handle)
{
	MeshResource& resource = meshResources[handle];
	resource.start = indices.size();

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	std::cout << resource.name << "\n";
	std::string PATH = "assets/models/" + resource.name + ".obj";
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, PATH.c_str())) {
		throw std::runtime_error(warn + err);
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			if (index.vertex_index < 0 ||
				index.texcoord_index < 0 ||
				index.normal_index < 0)
			{
				std::cout << "Invalid index detected\n";
			}
			Vertex vertex{};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);

			vertex.color = { 1.0f, 1.0f, 1.0f };
		}
	}
	resource.end = indices.size() - 1;
}

void AssetManager::createTexture(TextureHandle handle)
{
	createTextureImage(handle);
	createTextureImageView(handle);
	createTextureSampler(handle);
}

void AssetManager::createTextureImage(TextureHandle handle)
{
	Texture& texture = textureResources[handle];

	int texWidth, texHeight, texChannels;
	std::string PATH = "assets/textures/" + texture.name + ".png";

	stbi_uc* pixels = stbi_load(PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	VkDeviceSize imageSize = texWidth * texHeight * 4;

	texture.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VulkanHelper::createBuffer(context, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);
	void* data;
	vkMapMemory(context->device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(context->device, stagingBufferMemory);

	VulkanHelper::createImage(context, texWidth, texHeight, texture.mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture.image, texture.imageMemory, &texture.imageAllocation);
	VulkanHelper::transitionImageLayout(context, commandPool->get(), texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture.mipLevels);
	VulkanHelper::copyBufferToImage(context, commandPool->get(), stagingBuffer, texture.image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	VulkanHelper::generateMipmaps(context, commandPool->get(), texture.image, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, texture.mipLevels);

	vkDestroyBuffer(context->device, stagingBuffer, nullptr);
	vkFreeMemory(context->device, stagingBufferMemory, nullptr);
	stbi_image_free(pixels);
}

void AssetManager::createTextureImageView(TextureHandle handle)
{
	Texture& texture = textureResources[handle];

	texture.imageView = VulkanHelper::createImageView(context, texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, texture.mipLevels);
}

void AssetManager::createTextureSampler(TextureHandle handle)
{
	Texture& texture = textureResources[handle];

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(context->physicalDevice, &properties);
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = static_cast<float>(texture.mipLevels);

	if (vkCreateSampler(context->device, &samplerInfo, nullptr, &texture.sampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}

void AssetManager::createMaterialData(TextureHandle handle)
{
	// Load material before allocation to find if data is actually there
	loadMaterialData(handle);
	createMaterialBuffers(handle);
	for (int i = 0; i < *descriptorAllocator->MAX_FRAMES_IN_FLIGHT; i++) {
		updateMaterialBuffers(handle, i);
	}
}

void AssetManager::loadMaterialData(TextureHandle handle)
{
	Texture& texture = textureResources[handle];

	texture.materialData = glm::mat4(1.0);     // Dummy initialization as identity
}

void AssetManager::createMaterialBuffers(TextureHandle handle)
{
	Texture& texture = textureResources[handle];

	VkDeviceSize bufferSize = sizeof(LocalUniformBufferObject);

	texture.materialDataBuffers.resize(*(descriptorAllocator->MAX_FRAMES_IN_FLIGHT));
	texture.materialDataBuffersMemory.resize(*(descriptorAllocator->MAX_FRAMES_IN_FLIGHT));
	texture.materialDataBuffersMapped.resize(*(descriptorAllocator->MAX_FRAMES_IN_FLIGHT));

	for (size_t i = 0; i < *(descriptorAllocator->MAX_FRAMES_IN_FLIGHT); i++) {
		VulkanHelper::createBuffer(context, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &texture.materialDataBuffers[i], &texture.materialDataBuffersMemory[i]);

		vkMapMemory(context->device, texture.materialDataBuffersMemory[i], 0, bufferSize, 0, &texture.materialDataBuffersMapped[i]);
	}
}

void AssetManager::updateMaterialBuffers(TextureHandle handle, int currentFrame)
{
	Texture& texture = textureResources[handle];

	LocalUniformBufferObject ubo{};
	ubo.model = texture.materialData;
	memcpy(texture.materialDataBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}

void AssetManager::createTextureDescriptorSets(TextureHandle handle)
{
	Texture& texture = textureResources[handle];

	texture.descriptors.resize(*(descriptorAllocator->MAX_FRAMES_IN_FLIGHT));

	for (size_t i = 0; i < *(descriptorAllocator->MAX_FRAMES_IN_FLIGHT); i++) {
		descriptorAllocator->allocate(descriptorAllocator->layouts.meshAndSampler, texture.descriptors.data(), *(descriptorAllocator->MAX_FRAMES_IN_FLIGHT));

		VkDescriptorBufferInfo uniformBufferInfo{};
		uniformBufferInfo.buffer = texture.materialDataBuffers[i];
		uniformBufferInfo.offset = 0;
		uniformBufferInfo.range = sizeof(LocalUniformBufferObject);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = texture.imageView;
		imageInfo.sampler = texture.sampler;

		std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = texture.descriptors[i];
		descriptorWrites[0].dstBinding = 1;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(context->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void AssetManager::createIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VulkanHelper::createBuffer(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

	void* data;
	vkMapMemory(context->device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(context->device, stagingBufferMemory);

	VulkanHelper::createBuffer(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &indexBuffer, &indexBufferMemory);

	VulkanHelper::copyBuffer(context, commandPool->get(), stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(context->device, stagingBuffer, nullptr);
	vkFreeMemory(context->device, stagingBufferMemory, nullptr);
}

void AssetManager::createVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VulkanHelper::createBuffer(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

	void* data;
	vkMapMemory(context->device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(context->device, stagingBufferMemory);

	VulkanHelper::createBuffer(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer, &vertexBufferMemory);
	VulkanHelper::copyBuffer(context, commandPool->get(), stagingBuffer, vertexBuffer, bufferSize);

	vkDestroyBuffer(context->device, stagingBuffer, nullptr);
	vkFreeMemory(context->device, stagingBufferMemory, nullptr);
}

void AssetManager::cleanupObject(MeshHandle handle)
{
	MeshResource& resource = meshResources[handle];
	Texture& texture = textureResources[resource.texture];
	for (size_t i = 0; i < *(descriptorAllocator->MAX_FRAMES_IN_FLIGHT); i++) {
		vkDestroyBuffer(context->device, texture.materialDataBuffers[i], nullptr);
		vkFreeMemory(context->device, texture.materialDataBuffersMemory[i], nullptr);
	}

	vkDestroySampler(context->device, texture.sampler, nullptr);
	vkDestroyImageView(context->device, texture.imageView, nullptr);

	vmaDestroyImage(context->vmaAllocator, texture.image, texture.imageAllocation);
}

void AssetManager::cleanupFont(FontHandle handle)
{
	FontResource& resource = fontResources[handle];
	vkDestroyImageView(context->device, resource.atlas, nullptr);
	vkDestroyImage(context->device, resource.atlasImage, nullptr);
}

void AssetManager::createAtlasImage(FontResource& fontResource, Bitmap& bitmap)
{
	int atlasHeight = bitmap.height;
	int atlasWidth = bitmap.width;
	VkDeviceSize imageSize = atlasHeight * atlasWidth;
	std::cout << "\nImage Size: " << imageSize;
	VkBuffer imageStagingBuffer;
	VkDeviceMemory imageStagingBufferMemory;
	VulkanHelper::createBuffer(context, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &imageStagingBuffer, &imageStagingBufferMemory);

	void* data;
	vkMapMemory(context->device, imageStagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, bitmap.pixels.data(), static_cast<size_t>(imageSize));
	vkUnmapMemory(context->device, imageStagingBufferMemory);

	VulkanHelper::createImage(context, atlasWidth, atlasHeight, 1, VK_SAMPLE_COUNT_1_BIT, bitmap.format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, fontResource.atlasImage, fontResource.atlasImageMemory, &fontResource.atlasImageAllocation);

	std::cout << "\nStaging Buffer: " << reinterpret_cast<uint64_t>(imageStagingBuffer);
	VulkanHelper::transitionImageLayout(context, commandPool->get(), fontResource.atlasImage, bitmap.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
	VulkanHelper::copyBufferToImage(context, commandPool->get(), imageStagingBuffer, fontResource.atlasImage, static_cast<uint32_t>(atlasWidth), static_cast<uint32_t>(atlasHeight));
	VulkanHelper::transitionImageLayout(context, commandPool->get(), fontResource.atlasImage, bitmap.format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
	//VulkanHelper::generateMipmaps(pFontBundle->atlasImage, VK_FORMAT_R8_UINT, atlasWidth, atlasHeight, 1);

	vkDestroyBuffer(context->device, imageStagingBuffer, nullptr);
	vkFreeMemory(context->device, imageStagingBufferMemory, nullptr);
}

void AssetManager::createAtlasImageView(FontResource& fontResource, Bitmap& bitmap)
{
	createAtlasImage(fontResource, bitmap);
	fontResource.atlas = VulkanHelper::createImageView(context, fontResource.atlasImage, bitmap.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

Bitmap AssetManager::loadFontBitmap(const std::string& fontPath, int fontSize, FontResource& resource, int padding /*= 2*/)
{
	//std::cout << "Current working directory: " << std::filesystem::current_path() << "\n";
	//std::string fontPath2 = "fonts/times.ttf";
	std::ifstream file(fontPath, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	FT_Library ft;
	if (FT_Init_FreeType(&ft)) {
		throw std::runtime_error("Could not initialize FreeType");
	}

	FT_Face face;
	if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
		std::cout << "\nFT error code : " << FT_New_Face(ft, fontPath.c_str(), 0, &face);
		FT_Done_FreeType(ft);
		throw std::runtime_error("Could not load font");
	}

	FT_Set_Pixel_Sizes(face, 0, fontSize);

	const int firstChar = 32;
	const int lastChar = 126;
	const int numChars = lastChar - firstChar + 1;

	// Determine layout (grid size)
	int cols = 16;
	int rows = (numChars + cols - 1) / cols;

	unsigned int maxWidth = 0;
	unsigned int maxHeight = 0;

	// Get maximum glyph dimensions
	for (int c = firstChar; c <= lastChar; ++c) {
		if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;
		maxWidth = std::max(maxWidth, face->glyph->bitmap.width);
		maxHeight = std::max(maxHeight, face->glyph->bitmap.rows);
	}

	int cellWidth = maxWidth + padding;
	int cellHeight = maxHeight + padding;

	Bitmap bigBitmap(cols * cellWidth, rows * cellHeight);

	std::unordered_map<char, CharacterInfo> characters;

	for (int i = 0; i < numChars; ++i) {
		// Probably define character here?
		// Note that character is passed as a char, not an int! Should be somewhat easy to expand to any unicode character
		char c = firstChar + i;
		if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;

		int x = (i % cols) * cellWidth;
		int y = (i / cols) * cellHeight;
		bigBitmap.blit(face->glyph->bitmap, x, y);

		float atlasWidth = float(cols * cellWidth);
		float atlasHeight = float(rows * cellHeight);

		CharacterInfo character{};
		std::cout << "\nx: " << x;
		character.uv0[0] = x / atlasWidth;
		character.uv1[0] = (x + face->glyph->bitmap.width) / atlasWidth;
		character.uv0[1] = y / atlasHeight;
		character.uv1[1] = (y + face->glyph->bitmap.rows) / atlasHeight;
		character.width = face->glyph->bitmap.width;
		character.height = face->glyph->bitmap.rows;
		character.bearingX = int(face->glyph->metrics.horiBearingX / 64.0f);
		character.bearingY = int(face->glyph->metrics.horiBearingY / 64.0f);
		character.advance = int(face->glyph->metrics.horiAdvance / 64.0f);

		characters[i + firstChar] = character;
		resource.characters = characters;
	}

	FT_Done_Face(face);
	FT_Done_FreeType(ft);
	return bigBitmap;
}

void AssetManager::createAtlasDescriptorSet(FontResource& resource)
{
	resource.atlasDescriptorSet.resize(*(descriptorAllocator->MAX_FRAMES_IN_FLIGHT));
	descriptorAllocator->allocate(descriptorAllocator->layouts.fontSampler, resource.atlasDescriptorSet.data(), *descriptorAllocator->MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < *(descriptorAllocator->MAX_FRAMES_IN_FLIGHT); i++) {
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = resource.atlas;
		imageInfo.sampler = fontSampler;

		std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = resource.atlasDescriptorSet[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(context->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void AssetManager::createFontSampler()
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(context->physicalDevice, &properties);
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(context->device, &samplerInfo, nullptr, &fontSampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}

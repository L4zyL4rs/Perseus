#pragma once
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <array>
#include <string>
#include <mutex>
#include <iostream>
#include <unordered_map>
#include <filesystem>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cmath>
#include "PipelineBuilder.h"
#include "DescriptorAllocator.h"
#include "AssetManager.h"
#include "DrawItemAssembler.h"
#include "glm_config.h"


struct GlobalUniformBufferObject {
	glm::mat4 view;
	glm::mat4 proj;
	glm::vec4 light;
	glm::vec3 lightColor;
};

struct RenderObject {
	RenderObject(glm::vec3 pos, glm::vec3 vel, MeshHandle objectHandle, float objectScale = 1.0);
	glm::vec3 viewPosition{};
	glm::vec3 velocity;
	glm::vec3 scale;

	MeshHandle handle = UINT16_MAX;
};

// Class keeping track of different objects, their materials etc and setting up their required buffers
class ObjectManager
{
public:
	ObjectManager(DescriptorAllocator* dA, PipelineManager* pM, CommandPool* cP, AssetManager* aM, uint32_t* MFIF);
	//std::list<RenderObject> renderObjects;

	//void createObject();1
	//void deleteObject();
	void updateUniformBuffer(uint32_t currentImage);

	//void draw(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	//void drawUI(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	void cleanup();
	//void physics(std::atomic<bool>& MAIN_LOOP_RUNNING);
	void addObject(std::string ID, glm::vec3 position, glm::vec3 velocity, float objectScale = 1);

	const std::vector<DrawItem>& assembleDrawItems(uint32_t currentImage);

	// Having this openly accesible is bad, whole render structure is fucked rn though
	std::vector<VkDescriptorSet> globalDescriptorSets{};
private:
	RenderContext* context = nullptr;
	Swapchain* swapchain = nullptr;
	DescriptorAllocator* descriptorAllocator;
	PipelineManager* pipelineManager;
	AssetManager* assetManager;
	CommandPool* commandPool;
	uint32_t* MAX_FRAMES_IN_FLIGHT;
	AppWindow* window;
	std::array<const std::string, 2> MODEL_PATHS{ "models/viking_room.obj", "models/cube.obj" };
	std::array<const std::string, 2> TEXTURE_PATHS{ "textures/viking_room.png", "textures/cube.png" };

	std::vector<RenderObject> objects{};

	glm::vec3 viewCameraPos{};

	Int64vector lightPos{};
	glm::vec3 lightColor{1.0f};

	//std::vector<RenderObject> objects;
	std::vector<RenderObject> dummyObjects{};
	std::vector<uint32_t> meshIndices{};

	//struct GlobalUniformBufferObject;
	std::vector<VkBuffer> globalUniformBuffers{};
	std::vector<VkDeviceMemory> globalUniformBuffersMemory{};
	std::vector<void*> globalUniformBuffersMapped{};

	std::vector<DrawItem> drawItems{};

	std::thread physicsThread{};
	float gravity{};
	float physicsDelta = 1;		// Delta t for physics simulation in miliseconds

	void createGlobalUniformBuffers();
	void updateGlobalUniformBuffer(uint32_t currentImage);
	void createGlobalDescriptorSets();
	void createMeshGraphicsPipeline();
	std::vector<float> generateHitboxIntervals();
	void boxPhysics();
	void collisionPhysics();
};
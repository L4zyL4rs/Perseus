#include "FrameManager.h"
#include "SceneLoader.h"
#include "ECSView.h"
#include "systemBase.h"
#include <vector>

class RenderSystem : public ISystem{
public:
	RenderSystem(ecs::EntityManager& em) :
		entityManager(em),
		renderObjects(&em),
		textElements(&em),
		window(),
		context(&window),
		swapchain(&context),
		frameManager(&swapchain) {
		wrimels = 0;
	}

	std::vector<ecs::EntityBuilder>* run(uint32_t dt) {
		std::vector<ecs::EntityBuilder>* commands = new std::vector<ecs::EntityBuilder>;
		std::string sphere = "sphere";
		std::string cube = "cube";
		if (wrimels < 1) {
			wrimels++;
			commands->emplace_back(ecs::EntityBuilder(entityManager)
				.with<RenderObject>(RenderObject(glm::vec3(wrimels), glm::vec3(0), frameManager.assetManager.loadAsset(sphere))));
			commands->emplace_back(ecs::EntityBuilder(entityManager)
				.with<RenderObject>(RenderObject(glm::vec3(-wrimels), glm::vec3(0), frameManager.assetManager.loadAsset(cube))));
		}
		else {
			window.tick();
			assembleDrawItems();
			frameManager.draw(drawItems);
		}
		return commands;
	}

	void updateView() {
		renderObjects.update();
		textElements.update();
	}

private:
	ecs::EntityManager& entityManager;
	ecs::ECSView<RenderObject> renderObjects;
	ecs::ECSView<TextElement> textElements;
	AppWindow window;
	RenderContext context;
	Swapchain swapchain;
	FrameManager frameManager;
	std::vector<DrawItem> drawItems;
	int wrimels;

	void assembleDrawItems() {
		drawItems.clear();
		for (auto& [obj] : renderObjects) {
			assembleObjDrawItem(obj);
		}

		//assembleTextDrawItems();
	}

	void assembleObjDrawItem(RenderObject& obj) {
		// Asset Manager should not be buried inside of renderer, fine for now
		const VkBuffer& vertexBuffer = frameManager.assetManager.getVertexBuffer();
		const VkBuffer& indexBuffer = frameManager.assetManager.getIndexBuffer();

		const MeshResource& mesh = frameManager.assetManager.getMesh(obj.handle);
		frameManager.objectManager.updateUniformBuffer(frameManager.currentFrame);
		std::vector<VkDescriptorSet> descriptors;
		descriptors.push_back(frameManager.objectManager.globalDescriptorSets[frameManager.currentFrame]);
		descriptors.push_back(frameManager.assetManager.getDescriptors(mesh.texture)[frameManager.currentFrame]);

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), obj.viewPosition);
		transform = glm::translate(transform, -window.cam.worldCameraPos);
		transform = glm::rotate(transform, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		transform = glm::scale(transform, obj.scale);
		transform = glm::lookAt(window.cam.worldCameraPos, window.cam.cameraFront + window.cam.worldCameraPos, window.cam.cameraUp) * transform;
		glm::mat4 proj = glm::perspective(glm::radians(45.0f), swapchain.extent.width / (float)swapchain.extent.height, 0.1f, 10.0f);
		proj[1][1] *= -1;
		transform = proj * transform;

		std::cout << "xddddmeshstart " << mesh.start << "\n";

		DrawItem item;
		item.descriptorSets = descriptors;
		item.transform = transform;
		item.indexBuffer = indexBuffer;
		item.meshBuffer = vertexBuffer;
		item.meshStartIndex = mesh.start;
		item.meshStopIndex = mesh.end;
		item.pipeline = PipelineType::Mesh;
		item.sortKey = 0;

		//std::cout << "View position: \n" << object.viewPosition;
		//std::cout << "Push constant: \n" << item.transform;
		drawItems.push_back(item);
	}

	void assembleTextDrawItems() {
		std::unordered_map<FontHandle, std::vector<CharacterCoordinates>> texts;
		std::vector<CharacterCoordinates> textBuffer;

		for (auto& [element] : textElements) {
			const FontResource& resource = frameManager.assetManager.getFont(element.handle);
			texts[element.handle].append_range(element.text);
		}

		drawItems.append_range(frameManager.fontRenderer.assembleDrawItems(texts, textBuffer));
	};

	ecs::EntityBuilder addObject(std::string& name, glm::vec3 pos, glm::vec3 vel) {
		MeshHandle handle = frameManager.assetManager.loadAsset(name);
		ecs::EntityBuilder obj = ecs::EntityBuilder(entityManager).with<RenderObject>(RenderObject(pos, vel, handle));
		return obj;
	}
};
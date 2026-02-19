#include "FrameManager.h"
#include "GLFW/glfw3.h"
#include "SceneLoader.h"
#include "ECSView.h"
#include "systemBase.h"
#include "UserInput.h"
#include "Camera.h"
#include "EngineControl.h"
#include <iterator>
#include <vector>

class RenderSystem : public ISystem{
public:
	RenderSystem(ecs::EntityManager& em, uint32_t WINDOWWIDTH, uint32_t WINDOWHEIGHT) :
		entityManager(em),
		renderObjects(&em),
		textElements(&em),
        input(&em),
        camera(&em),
        control(&em),
		window(WINDOWWIDTH, WINDOWHEIGHT),
		context(&window),
		swapchain(&context),
		frameManager(&swapchain) {
		wrimels = 0;
	}

	std::vector<ecs::EntityBuilder>* run(uint32_t dt) override {
        queryInput();
        updateCamera(dt);
		std::vector<ecs::EntityBuilder>* commands = new std::vector<ecs::EntityBuilder>;
		window.tick();
		assembleDrawItems();
		frameManager.draw(drawItems);
		
		return commands;
	}

	void updateView() {
		renderObjects.update();
		textElements.update();
        input.update();
        camera.update();
        control.update();
	}

private:
	ecs::EntityManager& entityManager;
	ecs::ECSView<RenderObject> renderObjects;
	ecs::ECSView<TextElement> textElements;
    ecs::ECSView<UserInput> input;
    ecs::ECSView<Camera> camera;
    ecs::ECSView<EngineControl> control;
	AppWindow window;
	RenderContext context;
	Swapchain swapchain;
public:
	FrameManager frameManager;
private:
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
        auto itCam = camera.begin();
        auto& [cam] = *itCam;
		// Asset Manager should not be buried inside of renderer, fine for now
		const VkBuffer& vertexBuffer = frameManager.assetManager.getVertexBuffer();
		const VkBuffer& indexBuffer = frameManager.assetManager.getIndexBuffer();

		const MeshResource& mesh = frameManager.assetManager.getMesh(obj.handle);
		frameManager.objectManager.updateUniformBuffer(frameManager.currentFrame, cam.worldCameraPos, cam.cameraFront, cam.cameraUp);
		std::vector<VkDescriptorSet> descriptors;
		descriptors.push_back(frameManager.objectManager.globalDescriptorSets[frameManager.currentFrame]);
		descriptors.push_back(frameManager.assetManager.getDescriptors(mesh.texture)[frameManager.currentFrame]);

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), obj.viewPosition);
		transform = glm::translate(transform, -cam.worldCameraPos);
		transform = glm::rotate(transform, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		transform = glm::scale(transform, obj.scale);
		transform = glm::lookAt(cam.worldCameraPos, cam.cameraFront + cam.worldCameraPos, cam.cameraUp) * transform;
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

    void queryInput() {
        GLFWwindow* win = window.window;
        for(auto& [xd] : input) {std::cout << "hall\n";}
        std::cout << "x\n";
        auto it = input.begin();
        std::cout << "y\n";
        auto& [in] = *it;
        std::cout << "Hallo";

        in.keys[static_cast<size_t>(Key::w)] = (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS);
        in.keys[static_cast<size_t>(Key::a)] = (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS);
        in.keys[static_cast<size_t>(Key::s)] = (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS);
        in.keys[static_cast<size_t>(Key::d)] = (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS);
        in.keys[static_cast<size_t>(Key::space)] = (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS);
        in.keys[static_cast<size_t>(Key::esc)] = (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS);
        in.keys[static_cast<size_t>(Key::lShift)] = (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);
        in.keys[static_cast<size_t>(Key::lCtrl)] = (glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS);
        in.keys[static_cast<size_t>(Key::mouseL)] = (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
        in.keys[static_cast<size_t>(Key::mouseR)] = (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
        
        glfwGetCursorPos(win, &in.mouseX, &in.mouseY);
    }

    void updateCamera(float deltaTime) {
        auto itInput = input.begin();
        auto& [in] = *itInput;

        auto itCam = camera.begin();
        auto& [cam] = *itCam;
        
        auto itControl = control.begin();
        auto& [ctrl] = *itControl;

        bool wPressed = in.keys[static_cast<size_t>(Key::w)];
        bool aPressed = in.keys[static_cast<size_t>(Key::a)];
        bool sPressed = in.keys[static_cast<size_t>(Key::s)];
        bool dPressed = in.keys[static_cast<size_t>(Key::d)];
        bool lShiftPressed = in.keys[static_cast<size_t>(Key::lShift)];
        bool lCtrlPressed = in.keys[static_cast<size_t>(Key::lCtrl)];
        bool escPressed = in.keys[static_cast<size_t>(Key::esc)];

	    const float cameraSpeed = 0.0005f * deltaTime;
        std::cout << "Cam speed " << cameraSpeed << "\n";

        if(wPressed) {
            cam.worldCameraPos += cameraSpeed * cam.cameraFront;
        }
        if(sPressed) {
            cam.worldCameraPos -= cameraSpeed * cam.cameraFront;
        }
        if(aPressed) {
            cam.worldCameraPos -= cameraSpeed * glm::normalize(glm::cross(cam.cameraFront, cam.cameraUp));
        }
        if(dPressed) {
            cam.worldCameraPos += cameraSpeed * glm::normalize(glm::cross(cam.cameraFront, cam.cameraUp));
        }
        if(lShiftPressed) {
            cam.worldCameraPos += cameraSpeed * cam.cameraUp;
        }
        if(lCtrlPressed) {
            cam.worldCameraPos -= cameraSpeed * cam.cameraUp;
        }

        if(escPressed) {
            ctrl.requestExit = true;
        }

        double xOffset = in.mouseX - cam.lastX;
        double yOffset = in.mouseY - cam.lastY;
        cam.lastX = in.mouseX;
        cam.lastY = in.mouseY;
        
        xOffset *= MOUSESENSE;
        yOffset *= MOUSESENSE;

        cam.yaw += xOffset;
        cam.pitch += yOffset;

        if (cam.pitch > 89.0f)  {cam.pitch = 89.0f;}
    	if (cam.pitch < -89.0f) {cam.pitch = -89.0f;}

    	glm::vec3 direction;
	    direction.x = cos(glm::radians(-cam.yaw)) * cos(glm::radians(cam.pitch));
	    direction.y = sin(glm::radians(-cam.yaw)) * cos(glm::radians(cam.pitch));
	    direction.z = sin(glm::radians(cam.pitch));

	    cam.cameraFront = glm::normalize(direction);

    }
     
};

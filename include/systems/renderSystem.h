#pragma once
#include "Camera.h"
#include "ECSView.h"
#include "EngineControl.h"
#include "FrameManager.h"
#include "GLFW/glfw3.h"
#include "MeshGenerator.h"
#include "SceneLoader.h"
#include "SceneState.h"
#include "TextElement.h"
#include "UserInput.h"
#include "systemBase.h"
#include <glm/gtx/rotate_vector.hpp>
#include <iterator>
#include <string>
#include <vector>

class RenderSystem : public ISystem {
public:
  RenderSystem(ecs::EntityManager &em, uint32_t WINDOWWIDTH,
               uint32_t WINDOWHEIGHT, float vertFOV)
      : entityManager(em),
        renderObjects(&em),
        textElements(&em),
        input(&em),
        camera(&em),
        control(&em),
        lightSources(&em),
        window(WINDOWWIDTH, WINDOWHEIGHT),
        context(&window),
        swapchain(&context),
        frameManager(&swapchain),
        verticalFOV(vertFOV) {
    wrimels = 0;
  }

  std::vector<ecs::EntityBuilder> *run(uint32_t dt) override {
    updateDiagnostics();
    queryInput();
    updateScene(dt);
    std::vector<ecs::EntityBuilder> *commands =
        new std::vector<ecs::EntityBuilder>;
    window.tick();
    gatherFrameRenderInfo();
    frameManager.draw(renderInfo);

    return commands;
  }

  // Set up diagnostic texts
  std::vector<ecs::EntityBuilder> init() {
    std::vector<ecs::EntityBuilder> cmd;
    std::string robotoslab = "robotoslab";
    cmd.emplace_back(
        ecs::EntityBuilder(entityManager, &diagnostics)
            .with<TextElement128>(TextElement128(
                "text", frameManager.assetManager.loadFont(robotoslab, 30),
                glm::vec2(0, 0), glm::vec4(1))));
    LightSource light1({10, 0, 0, 0}, {0, 1, 0, 0.01});
    cmd.emplace_back(ecs::EntityBuilder(entityManager).with(light1));
    LightSource light2({0, 10, 0, 0}, {0, 0, 1, 0.01});
    cmd.emplace_back(ecs::EntityBuilder(entityManager).with(light2));
    LightSource light3({0, 0, 10, 0}, {1, 0, 0, 0.01});
    cmd.emplace_back(ecs::EntityBuilder(entityManager).with(light3));
    LightSource light4({0, 0, 10, 0}, {1, 1, 1, 0.7});
    cmd.emplace_back(ecs::EntityBuilder(entityManager).with(light4));

    return cmd;
  }

  // Maybe better to set up a queue of callback functions?
  void updateView() override {
    renderObjects.update();
    textElements.update();
    input.update();
    camera.update();
    control.update();
    lightSources.update();
  }

private:
  ecs::EntityManager &entityManager;
  ecs::ECSView<RenderObject> renderObjects;
  ecs::ECSView<TextElement128> textElements;
  ecs::ECSView<UserInput> input;
  ecs::ECSView<Camera> camera;
  ecs::ECSView<EngineControl> control;
  ecs::ECSView<LightSource> lightSources;
  AppWindow window;
  RenderContext context;
  Swapchain swapchain;

public:
  FrameManager frameManager;
  float verticalFOV;

private:
  int wrimels;
  ecs::Entity diagnostics;
  FrameRenderInfo renderInfo;

  void gatherFrameRenderInfo() {
    gatherCameraData();
    gatherLights();
    gatherRenderInstances();
    gatherLights();
  }

  void gatherRenderInstances() {
    renderInfo.meshes.clear();
    for(auto& [object] : renderObjects) {
      RenderInstance instance;
      instance.mesh = object.handle;
      instance.transform = glm::translate(glm::mat4(1.0f), object.viewPosition);
      instance.flags.castsShadow = true;
      renderInfo.meshes.push_back(instance);
    }
  }

  void gatherCameraData() {
    auto& [cam] = *camera.begin();

    renderInfo.camPos = cam.worldCameraPos;
    renderInfo.camForward = cam.cameraFront;
    renderInfo.camUp = cam.cameraUp;
    renderInfo.verticalFOV = cam.verticalFOV;

    renderInfo.windowWidth = window.WINDOWWIDTH;
    renderInfo.windowHeight = window.WINDOWHEIGHT;

    renderInfo.viewMatrix = glm::lookAt(cam.worldCameraPos,
                            cam.worldCameraPos + cam.cameraFront,
                            cam.cameraUp);

    float aspect = static_cast<float>(window.WINDOWWIDTH) /
                   static_cast<float>(window.WINDOWHEIGHT);

    renderInfo.projMatrix = glm::perspective(glm::radians(cam.verticalFOV), aspect, 0.1f, 100.0f);

    renderInfo.projMatrix[1][1] *= -1.0f;

    float verticalRadians = glm::radians(cam.verticalFOV);
    float horizontalRadians = 2.0f * std::atan(std::tan(verticalRadians * 0.5f) * aspect);

    renderInfo.horizontalFOV = glm::degrees(horizontalRadians);
}

  void gatherLights() {
    renderInfo.lights.clear();
    for(auto& [light] : lightSources) {
      renderInfo.lights.emplace_back(light);
    }
  }
  // Maybe create renderSystem-local TextElement and not upload to ECS
  // Force upload of valid drawItems into renderer, see if other ECS values also
  // get overwritten
  void gatherTextElements() {
    renderInfo.texts.clear();
    for (const auto &[element] : textElements) {
      // const FontResource& resource =
      // frameManager.assetManager.getFont(element.handle);
      renderInfo.texts[element.handle].append_range(toCharacterCooordinates(element));
      //std::cout << "Character coordinates count is "
      //          << texts[element.handle].size() << "\n";
      //std::cout << "Text is now " << element.text << "\n";
    }

    //renderInfo.meshItems.append_range(frameManager.fontRenderer.assembleDrawItems(
    //    texts, frameManager.currentFrame));
    //for (const auto [element] : textElements) {
      //std::cout << "Text is " << element.text << "\n";
    //}
  };

  // Generates the corresponding CharacterCoordinates structs belonging to the
  // text input, and writes them to the TextElement Build the vertices like
  // this:
  //
  //       v0-----v1
  //       |     / |
  //       |   /   |
  //       | /     |
  //       v2-----v3
  //
  std::vector<CharacterCoordinates>
  toCharacterCooordinates(const TextElement128 &in) {
    std::vector<CharacterCoordinates> coords;
    const FontResource &font = frameManager.assetManager.getFont(in.handle);
    coords.reserve(128 * 6);

    glm::vec2 pen = in.screenPosUv;
    glm::vec2 rect = glm::vec2(window.WINDOWWIDTH, window.WINDOWHEIGHT);
    //std::cout << "Try to load " << in.text << "\n";

    for (char ch : in.text) {
      if (ch == 0) {
        break;
      }
      // for(auto& letter : pTextElement->fontBundle->characters)
      //std::cout << "\nLoading character: " << (int)ch << std::flush;
      const CharacterInfo &character = font.characters.at(ch);

      glm::vec2 yBearing = glm::vec2(0.0, -character.bearingY) / rect;

      // Screen space positions
      glm::vec2 topLeft = pen + yBearing;
      glm::vec2 topRight =
          pen + glm::vec2(character.width, 0.0) / rect + yBearing;
      glm::vec2 bottomLeft =
          pen + glm::vec2(0.0, character.height) / rect + yBearing;
      glm::vec2 bottomRight =
          pen + glm::vec2(character.width, character.height) / rect + yBearing;

      // std::cout << "\nPen TopLeft: " << topLeft.x << "\nPen BottomRight: " <<
      // bottomRight.x;

      // Texture UVs
      glm::vec2 uv0 = character.uv0;
      glm::vec2 uv1 = glm::vec2(character.uv1.x, character.uv0.y);
      glm::vec2 uv2 = glm::vec2(character.uv0.x, character.uv1.y);
      ;
      glm::vec2 uv3 = character.uv1;
      glm::vec4 color =
          glm::vec4(in.color); // Make this more elaborate some day

      // std::cout << "\nTexture x TopLeft: " << uv0.x << "\nTexture
      // BottomRight: " << uv3.x;

      // First triangle
      // coordinates.push_back({ uv0,    topLeft,     color });
      // coordinates.push_back({ uv1,    topRight,    color });
      // coordinates.push_back({ uv2,    bottomLeft,  color });

      // Second triangle
      // coordinates.push_back({ uv1,    topRight,    color });
      // coordinates.push_back({ uv2,    bottomLeft,  color });
      // coordinates.push_back({ uv3,    bottomRight, color });

      // Triangle 1
      coords.push_back({uv0, topLeft, color});
      coords.push_back({uv2, bottomLeft, color});
      coords.push_back({uv3, bottomRight, color});

      // Triangle 2
      coords.push_back({uv0, topLeft, color});
      coords.push_back({uv3, bottomRight, color});
      coords.push_back({uv1, topRight, color});

      pen.x += float(character.advance) / window.WINDOWWIDTH;
    }
    return coords;
  }

  ecs::EntityBuilder addObject(std::string &name, glm::vec3 pos,
                               glm::vec3 vel) {
    MeshHandle handle = frameManager.assetManager.loadAsset(name);
    ecs::EntityBuilder obj =
        ecs::EntityBuilder(entityManager)
            .with<RenderObject>(RenderObject(pos, vel, handle));
    return obj;
  }

  void queryInput() {
    GLFWwindow *win = window.window;
    auto it = input.begin();
    auto &[in] = *it;

    in.keys[static_cast<size_t>(Key::w)] =
        (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS);
    in.keys[static_cast<size_t>(Key::a)] =
        (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS);
    in.keys[static_cast<size_t>(Key::s)] =
        (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS);
    in.keys[static_cast<size_t>(Key::d)] =
        (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS);
    in.keys[static_cast<size_t>(Key::space)] =
        (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS);
    in.keys[static_cast<size_t>(Key::esc)] =
        (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS);
    in.keys[static_cast<size_t>(Key::lShift)] =
        (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);
    in.keys[static_cast<size_t>(Key::lCtrl)] =
        (glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS);
    in.keys[static_cast<size_t>(Key::mouseL)] =
        (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    in.keys[static_cast<size_t>(Key::mouseR)] =
        (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);

    glfwGetCursorPos(win, &in.mouseX, &in.mouseY);
  }

  void updateCamera(float deltaTime) {
    auto itInput = input.begin();
    auto &[in] = *itInput;

    auto itCam = camera.begin();
    auto &[cam] = *itCam;

    auto itControl = control.begin();
    auto &[ctrl] = *itControl;

    bool wPressed = in.keys[static_cast<size_t>(Key::w)];
    bool aPressed = in.keys[static_cast<size_t>(Key::a)];
    bool sPressed = in.keys[static_cast<size_t>(Key::s)];
    bool dPressed = in.keys[static_cast<size_t>(Key::d)];
    bool lShiftPressed = in.keys[static_cast<size_t>(Key::lShift)];
    bool lCtrlPressed = in.keys[static_cast<size_t>(Key::lCtrl)];
    bool escPressed = in.keys[static_cast<size_t>(Key::esc)];

    const float cameraSpeed = 0.0005f * deltaTime;
    //std::cout << "Cam speed " << cameraSpeed << "\n";

    if (wPressed) {
      cam.worldCameraPos += cameraSpeed * cam.cameraFront;
    }
    if (sPressed) {
      cam.worldCameraPos -= cameraSpeed * cam.cameraFront;
    }
    if (aPressed) {
      cam.worldCameraPos -=
          cameraSpeed *
          glm::normalize(glm::cross(cam.cameraFront, cam.cameraUp));
    }
    if (dPressed) {
      cam.worldCameraPos +=
          cameraSpeed *
          glm::normalize(glm::cross(cam.cameraFront, cam.cameraUp));
    }
    if (lShiftPressed) {
      cam.worldCameraPos += cameraSpeed * cam.cameraUp;
    }
    if (lCtrlPressed) {
      cam.worldCameraPos -= cameraSpeed * cam.cameraUp;
    }

    if (escPressed) {
      ctrl.requestExit = true;
    }

    double xOffset = in.mouseX - cam.lastX;
    double yOffset = in.mouseY - cam.lastY;
    cam.lastX = in.mouseX;
    cam.lastY = in.mouseY;

    xOffset *= MOUSESENSE;
    yOffset *= MOUSESENSE;

    cam.yaw += xOffset;
    cam.pitch -= yOffset;

    if (cam.pitch > 89.0f) {
      cam.pitch = 89.0f;
    }
    if (cam.pitch < -89.0f) {
      cam.pitch = -89.0f;
    }

    glm::vec3 direction;
    direction.x = cos(glm::radians(-cam.yaw)) * cos(glm::radians(cam.pitch));
    direction.y = sin(glm::radians(-cam.yaw)) * cos(glm::radians(cam.pitch));
    direction.z = sin(glm::radians(cam.pitch));

    cam.cameraFront = glm::normalize(direction);

    float v = glm::radians(cam.verticalFOV);
    float aspect = window.WINDOWWIDTH / float(window.WINDOWHEIGHT);
    float h = 2.0f * atan(tan(v * 0.5f) * aspect);
  }

  void updateDiagnostics() {
    std::string text = "Frame " + std::to_string(frameManager.currentFrame);
    entityManager.getComponent<TextElement128>(diagnostics).change(text);
  }

  glm::mat4 calculateViewMatrix() {
    auto itCam = camera.begin();
    auto &[cam] = *itCam;
    glm::mat4 transform =
        glm::lookAt(cam.worldCameraPos, cam.cameraFront + cam.worldCameraPos,
                    cam.cameraUp);
    return transform;
  }

  glm::mat4 calculateProjMatrix() {
    glm::mat4 proj = glm::perspective(
        glm::radians(45.0f),
        swapchain.extent.width / (float)swapchain.extent.height, 0.1f, 100.0f);
    proj[1][1] *= -1;
    
    return proj;
  }

  void updateScene(float dt) {
    updateCamera(dt);

    float angle = dt / 1000;
    lightSources.at(0).pos = glm::rotateZ(lightSources.at(0).pos, angle);
    lightSources.at(1).pos = glm::rotateX(lightSources.at(1).pos, angle);
    lightSources.at(2).pos = glm::rotateY(lightSources.at(2).pos, angle);

  }

};

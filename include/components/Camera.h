#pragma once
#include "glm_config.h"

struct Camera {
    // lastX and lastY should be set to center of screen at start
    float lastX{};
    float lastY{};
    float yaw = 0.0f;
    float pitch = 0.0f;
    glm::vec3 cameraFront = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 worldCameraPos = glm::vec3(0, 0, 0);
};

#pragma once
#include "glm_config.h"

#include <int64vector.h>

extern uint32_t WINDOWWITH_INIT;
extern uint32_t WINDOWHEIGHT_INIT;

struct Camera {
    float lastX = WINDOWWITH_INIT / 2;
    float lastY = WINDOWHEIGHT_INIT / 2;
    float yaw = 0.0f;
    float pitch = 0.0f;
    glm::vec3 cameraFront = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 worldCameraPos = glm::vec3(0, 0, 0);
};
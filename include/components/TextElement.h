#include <cstring>
#include <string>
#include "glm_config.h"
#include <iostream>

struct TextElement128 {
    char text[128];
    uint16_t handle;
    glm::vec2 screenPosUv;
    glm::vec4 color;
    TextElement128(std::string t, uint16_t h, glm::vec2&& s, glm::vec4&& c) :
        text(),
        handle(h),
        screenPosUv(s),
        color(c)
    {
        std::strncpy(text, t.c_str(), 128);
    }

    void change(std::string t) {
        std::strncpy(text, t.c_str(), 128);
    }
};

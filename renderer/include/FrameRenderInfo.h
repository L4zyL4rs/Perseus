#include <vector>
#include <stdint.h>
#include "DrawItemAssembler.h"

struct FrameRenderInfo {
  float verticalFOV;
  float horizontalFOV;
  uint32_t windowHeight;
  uint32_t windowWidth;
  glm::vec3 camPos;
  glm::vec3 camForward;
  glm::vec3 camUp;
  std::vector<DrawItem> meshItems;
};


#include <unordered_map>
#include <vector>
#include <stdint.h>
#include "AssetManager.h"
#include "DrawItemAssembler.h"
#include "RenderInstance.h"
#include "Light.h"

// Complete abstract description of frame
// All scene-specific data MUST be communicated through this struct
// Consider using std::pmr in the future to make this into one contiguous memory block
struct FrameRenderInfo {
  float verticalFOV;
  float horizontalFOV;
  uint32_t windowHeight;
  uint32_t windowWidth;
  glm::vec3 camPos;
  glm::vec3 camForward;
  glm::vec3 camUp;
  glm::mat4 viewMatrix{1.0f};
  glm::mat4 projMatrix{1.0f};
  std::vector<LightSource> lights;
  std::vector<RenderInstance> meshes;
  std::unordered_map<FontHandle, std::vector<CharacterCoordinates>> texts;
};


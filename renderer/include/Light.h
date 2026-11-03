#pragma once
#include <ostream>
#include "glm_config.h"

enum class LightSourceType {
  Orthographic,
  Point,
  Directional
};

struct LightSource {
public:
  void updatePos(glm::vec4 newPos) {
    pos = newPos;
  }

  void updateCol(glm::vec4 newCol) {
    col = newCol;
  }

  glm::vec4 pos{};
  glm::vec4 col{};
  glm::mat4 view{};
  glm::mat4 proj{};
};



inline std::ostream& operator<<(std::ostream& out, const LightSource in) {
    return out << "Light source at " << 
           in.pos.x << " " << in.pos.y << " " << in.pos.z << " " << 
           " with colour " << 
           in.col.x << " " << in.col.y << " " << in.col.z << " " << in.col.w
           << "\n";
}

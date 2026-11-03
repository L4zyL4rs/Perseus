#pragma once
#include "AssetManager.h"
#include "glm/ext/matrix_float4x4.hpp"

struct RenderFlags {
  bool castsShadow = true;
};

// Abstract description of object that is to be rendered
struct RenderInstance {
  MeshHandle mesh;
  glm::mat4 transform;
  RenderFlags flags;
};


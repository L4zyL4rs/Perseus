#version 450

struct LightSource {
  vec4 pos;
  vec4 col;
  mat4 lightView;
  mat4 lightProj;
};

layout(binding = 0) uniform GlobalUniformBufferObject {
// Pay attention to correct order of variables in this struct!
    vec4 pos;
    vec4 up;
    vec4 front;
    mat4 view;
    mat4 proj;
    ivec4 lightCount; 
    LightSource lightSources[10];
} uboGlobal;

layout(location=0) in vec3 inPosition;

layout(push_constant) uniform constants {
  mat4 transform;
} pushConstant;

void main() {
  LightSource light = uboGlobal.lightSources[0];
  gl_Position = light.lightProj 
              * light.lightView
              * pushConstant.transform
              * vec4(inPosition, 1.0);
}


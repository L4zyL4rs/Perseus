#version 450

struct LightSource {
    vec4 pos;
    vec4 col;
};

layout(binding = 0) uniform GlobalUniformBufferObject {
// Pay attention to correct order of variables in this struct!
    vec4 pos;
    vec4 up;
    vec4 front;
    ivec4 lightCount; 
    LightSource lightSources[10];
} uboGlobal;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(push_constant) uniform constants {
    mat4 transform;
} pushConstant;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 fragPosition;

void main() {
    fragPosition = vec4(inPosition, 1.0f);
    //gl_Position = uboGlobal.proj * uboGlobal.view * fragPosition;
    gl_Position = pushConstant.transform * vec4(inPosition, 1.0f);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}

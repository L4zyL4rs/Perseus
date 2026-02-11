#version 450

layout(binding = 0) uniform GlobalUniformBufferObject {
// Pay attention to correct order of variables in this struct!
    mat4 view;
    mat4 proj;
    vec4 light;
    vec3 lightColor;
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
    //fragPosition = pushConstant.transform * vec4(inPosition, 1.0f);
    //gl_Position = uboGlobal.proj * uboGlobal.view * fragPosition;
    gl_Position = pushConstant.transform * vec4(inPosition, 1.0f);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
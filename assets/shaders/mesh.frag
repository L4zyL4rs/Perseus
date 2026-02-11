#version 450

layout(binding = 0) uniform GlobalUniformBufferObject {
// Pay attention to correct order of variables in this struct!
    mat4 view;
    mat4 proj;
    vec4 light;
    vec3 lightColor;
} uboGlobal;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragNormal;
layout(location = 3) in vec4 fragPos;

layout(set = 1, binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 lightDirection = normalize(uboGlobal.light - fragPos);
    outColor = vec4((0 * dot(lightDirection.xyz, fragNormal.xyz) + 1.0) * texture(texSampler, fragTexCoord).xyz * uboGlobal.lightColor, 0.8);
}
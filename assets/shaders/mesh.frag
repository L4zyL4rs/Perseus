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

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragNormal;
layout(location = 3) in vec4 fragPos;

layout(set = 1, binding = 1) uniform sampler2D texSampler;

layout(push_constant) uniform constants {
    mat4 transform;
} pushConstant;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(0, 0, 0, 0);
   
    // Keep in mind lightCount is an ivec4 for alignment
    for(int i = 0; i < uboGlobal.lightCount.x; i++) {
        LightSource source = uboGlobal.lightSources[i];

        vec4 lightDirection = normalize(source.pos - fragPos);
        outColor += vec4((0.8 * dot(lightDirection.xyz, fragNormal.xyz) + 0.0) * texture(texSampler, fragTexCoord).xyz * source.col.xyz, 0.8);
    }
}


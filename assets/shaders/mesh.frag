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

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragNormal;
layout(location = 3) in vec4 fragPos;

layout(set = 1, binding = 1) uniform sampler2D texSampler;
layout(set = 2, binding = 0) uniform sampler2D shadowMap;

layout(push_constant) uniform constants {
    mat4 transform;
} pushConstant;

layout(location = 0) out vec4 outColor;

float ambientLight = 0.05f;
float bias = 0.008;

void main() {
    outColor = vec4(0, 0, 0, 0);
   
    // Keep in mind lightCount is an ivec4 for alignment
    for(int i = 0; i < uboGlobal.lightCount.x; i++) {
        LightSource light = uboGlobal.lightSources[i];
        vec4 lightClip = light.lightProj * light.lightView * fragPos;
        vec3 lightNdc = lightClip.xyz / lightClip.w;
        vec2 shadowUv = lightNdc.xy * 0.5 + 0.5;

        float storedDepth = texture(shadowMap, shadowUv).r;
        float shadow = lightNdc.z - bias > storedDepth ? 1.0 : 0.0;

        vec4 lightDirection = normalize(light.pos);
        outColor += (1.0 - shadow) * vec4((0.8 * max(0.0f, dot(lightDirection.xyz, fragNormal.xyz)) + 0.0f) * texture(texSampler, fragTexCoord).xyz * light.col.rgb * light.col.a, 1.0);
    }
    outColor.rgb = outColor.rgb * fragColor;
    outColor += ambientLight * texture(texSampler, fragTexCoord);
}


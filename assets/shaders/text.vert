#version 450
layout (location = 0) in vec2 textureUV;
layout (location = 1) in vec2 screenUV;
layout (location = 2) in vec4 inColor;

layout (location = 0) out vec2 uv;
layout (location = 1) out vec4 outColor;


// layout (binding = 0) uniform Ubo {
//     mat4 projection;
// } projectionUbo;

void main()
{
    gl_Position = vec4(screenUV, 0.0, 1.0);
    uv = textureUV;
    outColor = inColor;
}  
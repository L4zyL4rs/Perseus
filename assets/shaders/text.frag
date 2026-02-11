#version 450
layout (location=0) in vec2 TexCoords;
layout (location=0) out vec4 color;

layout(set = 0, binding = 0) uniform sampler2D atlas;

void main()
{    
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(atlas, TexCoords).r);
    color = vec4(1.0, 1.0, 1.0, 1.0) * sampled;
}  
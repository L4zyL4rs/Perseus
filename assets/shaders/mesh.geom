#version 450

// TODO: Automcatic shader compilation instead of manual

// Inputs and outputs of vertex data
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

// Data going unchanged to fragment shader
layout (location = 0) in vec3 vertColor[];
layout (location = 1) in vec2 vertTexCoord[];
layout (location = 2) in vec4 vertFragPosition[];

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec2 fragTexCoord;
layout (location = 2) out vec4 fragNormal;
layout (location = 3) out vec4 fragPosition;

void main() {
	// Span triangle from two vectors, then calculate normal of triangle
	vec3 base1 = vertFragPosition[1].xyz - vertFragPosition[0].xyz;
	vec3 base2 = vertFragPosition[2].xyz - vertFragPosition[0].xyz;
	vec4 normal = vec4(normalize(cross(base1, base2)), 0);

	for (int i = 0; i < 3; i++) {
		gl_Position = gl_in[i].gl_Position;
		fragColor = vertColor[i];
		fragTexCoord = vertTexCoord[i];
		fragNormal = normal;
		fragPosition = vertFragPosition[i];
		EmitVertex();
	}

	EndPrimitive();
}

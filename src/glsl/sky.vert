#version 460
#pragma shader_stage(vertex)

layout(location = 0) out vec4 f_position;

layout(binding = 0) uniform World {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	vec4 lightPosition;
	vec4 lightColor;
	vec4 ambientColor;
} world;

#include "util.glslh"

void main() {
	vec3 position = cubeVertex(gl_VertexIndex) * 2.0 - 1.0;
	f_position = vec4(position, 0.0);
	gl_Position = world.viewProjection * f_position;
}

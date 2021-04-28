#version 460
#pragma shader_stage(fragment)

layout(location = 0) in vec4 f_position;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform World {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
} world;

layout(binding = 1) uniform samplerCube cubemap;

void main() {
	out_color = vec4(textureLod(cubemap, normalize(f_position.rgb), 0.0).rgb, 1.0);
}

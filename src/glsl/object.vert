#version 460
#pragma shader_stage(vertex)

layout(location = 0) in vec4 v_position;
layout(location = 1) in vec4 v_normal;
layout(location = 2) in vec4 v_color;

layout(location = 0) out flat uint InstanceIndex;
layout(location = 1) out vec4 f_position;
layout(location = 2) out vec4 f_color;
layout(location = 3) out vec4 f_normal;
layout(location = 4) out vec4 f_lightPosition; // in view space
layout(location = 5) out vec4 f_lightSpacePosition;

#include "object.glslh"

layout(binding = 3) uniform LightProjection {
	mat4 lightProjection;
};

void main() {
	InstanceIndex = gl_InstanceIndex;
	const Instance instance = instances.data[InstanceIndex];

	gl_Position = world.viewProjection * instance.transform * v_position;
	f_position = world.view * instance.transform * v_position;
	f_color = v_color * instance.tint;
	f_normal = vec4(mat3(transpose(inverse(world.view * instance.transform))) * v_normal.xyz, 0.0);
	f_lightPosition = world.view * world.lightPosition;
	f_lightSpacePosition = lightProjection * instance.transform * v_position;
}

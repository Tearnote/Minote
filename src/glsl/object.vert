#version 460
#pragma shader_stage(vertex)

layout(location = 0) in vec4 v_position;
layout(location = 1) in vec4 v_normal;
layout(location = 2) in vec4 v_color;

layout(location = 0) out flat uint InstanceIndex;
layout(location = 1) out vec3 f_position;
layout(location = 2) out vec4 f_color;
layout(location = 3) out vec3 f_normal;
layout(location = 4) out vec3 f_viewPosition;

#include "object.glslh"

void main() {
	InstanceIndex = gl_InstanceIndex;
	const Instance instance = instances.data[InstanceIndex];

	gl_Position = world.viewProjection * instance.transform * v_position;
	f_position = vec3(instance.transform * v_position);
	f_color = v_color * instance.tint;
	f_normal = mat3(transpose(inverse(instance.transform))) * v_normal.xyz;
	f_viewPosition = vec3(inverse(world.view) * vec4(0.0, 0.0, 0.0, 1.0));
}

#version 460
#pragma shader_stage(vertex)

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec4 v_color;

layout(location = 0) out flat uint InstanceIndex;
layout(location = 1) out vec3 f_position;
layout(location = 2) out vec4 f_color;
layout(location = 3) out vec3 f_normal;
layout(location = 4) out vec3 f_viewPosition;

#include "object.glsl"

void main() {
	InstanceIndex = gl_InstanceIndex;
	const Instance instance = instances.data[InstanceIndex];

	gl_Position = world.viewProjection * instance.transform * vec4(v_position, 1.0);
	f_position = vec3(instance.transform * vec4(v_position, 1.0));
	f_color = v_color * instance.tint;
	f_normal = mat3(transpose(inverse(instance.transform))) * v_normal;
	f_viewPosition = vec3(inverse(world.view) * vec4(0.0, 0.0, 0.0, 1.0));
}

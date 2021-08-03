#version 460
#pragma shader_stage(vertex)

#include "types.glsl"

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec4 v_color;

layout(location = 0) out flat uint InstanceIndex;
layout(location = 1) out vec3 f_position;
layout(location = 2) out vec4 f_color;
layout(location = 3) out vec3 f_normal;
layout(location = 4) out vec3 f_viewPosition;

layout(std430, binding = 1) readonly buffer RowTransforms {
	RowTransform transforms[];
};
layout(std430, binding = 2) readonly buffer Materials {
	Material materials[];
};

#include "world.glsl"

void main() {
	InstanceIndex = gl_InstanceIndex;
	const mat4 transform = getTransform(transforms[gl_InstanceIndex]);

	gl_Position = world.viewProjection * transform * vec4(v_position, 1.0);
	f_position = vec3(transform * vec4(v_position, 1.0));
	f_color = v_color * materials[InstanceIndex].tint;
	f_normal = mat3(transpose(inverse(transform))) * v_normal;
	f_viewPosition = vec3(inverse(world.view) * vec4(0.0, 0.0, 0.0, 1.0));
}

#version 460
#pragma shader_stage(vertex)

#include "types.glsl"

layout(location = 0) in vec3 v_position;

layout(std430, binding = 1) readonly buffer RowTransforms {
	RowTransform transforms[];
};

#include "world.glsl"

void main() {
	gl_Position = world.viewProjection * getTransform(transforms[gl_InstanceIndex]) * vec4(v_position, 1.0);
}

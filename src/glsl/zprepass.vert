#version 460
#pragma shader_stage(vertex)

#include "types.glsl"
#include "world.glsl"

layout(location = 0) in vec3 v_position;

layout(binding = 0) uniform WorldConstants {
	World world;
};

layout(std430, binding = 1) readonly buffer RowTransforms {
	RowTransform transforms[];
};

void main() {
	gl_Position = world.viewProjection * getTransform(transforms[gl_InstanceIndex]) * vec4(v_position, 1.0);
}

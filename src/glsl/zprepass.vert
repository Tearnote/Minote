#version 460
#pragma shader_stage(vertex)

#include "types.glsl"

layout(location = 0) in vec3 v_position;

layout(binding = 0) uniform WorldConstants {
	World u_world;
};
layout(std430, binding = 1) readonly buffer RowTransforms {
	RowTransform b_transforms[];
};

void main() {
	
	const mat4 transform = getTransform(b_transforms[gl_InstanceIndex]);
	gl_Position = u_world.viewProjection * transform * vec4(v_position, 1.0);
	
}

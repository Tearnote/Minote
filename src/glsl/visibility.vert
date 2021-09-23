#version 460
#pragma shader_stage(vertex)

#include "types.glsl"

layout(location = 0) out flat uint InstanceIndex;

layout(binding = 0) uniform WorldConstants {
	World u_world;
};
layout(std430, binding = 1) readonly buffer Vertices {
	float b_vertices[];
};
layout(std430, binding = 2) readonly buffer Transforms {
	Transform b_transforms[];
};

void main() {
	
	InstanceIndex = gl_InstanceIndex;
	
	vec3 v_position = vec3(
		b_vertices[gl_VertexIndex*3 + 0],
		b_vertices[gl_VertexIndex*3 + 1],
		b_vertices[gl_VertexIndex*3 + 2]);
	
	const mat4 transform = getTransform(b_transforms[gl_InstanceIndex]);
	gl_Position = u_world.viewProjection * transform * vec4(v_position, 1.0);
	
}

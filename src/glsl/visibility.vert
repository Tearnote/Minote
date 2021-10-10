#version 460
#pragma shader_stage(vertex)

#include "types.glsl"

#define B_VERTICES b_vertices

layout(location = 0) out flat uint InstanceIdx;

layout(binding = 0) uniform WorldConstants {
	World u_world;
};
layout(std430, binding = 1) readonly buffer Vertices {
	float B_VERTICES[];
};
layout(std430, binding = 2) readonly buffer Transforms {
	Transform b_transforms[];
};

#include "typesAccess.glsl"

void main() {
	
	InstanceIdx = gl_InstanceIndex;
	
	vec3 vertex = fetchVertex(gl_VertexIndex);
	
	mat4 transform = getTransform(b_transforms[gl_InstanceIndex]);
	gl_Position = u_world.viewProjection * transform * vec4(vertex, 1.0);
	
}

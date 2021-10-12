#version 460
#pragma shader_stage(vertex)

#include "types.glsl"

#define B_VERTICES b_vertices

layout(location = 0) out flat uint InstanceIdx;

layout(binding = 0) uniform WorldConstants {
	World u_world;
};
layout(std430, binding = 1) restrict readonly buffer Vertices {
	float B_VERTICES[];
};
layout(std430, binding = 2) restrict readonly buffer Instances {
	Instance b_instances[];
};
layout(std430, binding = 3) restrict readonly buffer Transforms {
	mat3x4 b_transforms[];
};

#include "typesAccess.glsl"

void main() {
	
	InstanceIdx = gl_InstanceIndex;
	
	vec3 vertex = fetchVertex(gl_VertexIndex);
	
	uint transformIdx = b_instances[gl_InstanceIndex].transformIdx;
	mat4 transform = getTransform(b_transforms[transformIdx]);
	gl_Position = u_world.viewProjection * transform * vec4(vertex, 1.0);
	
}

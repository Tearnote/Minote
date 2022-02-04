#version 460
#pragma shader_stage(vertex)

#include "../types.glsl"

#define B_VERTICES b_vertices

layout(binding = 0) uniform WorldConstants {
	World u_world;
};
layout(binding = 1, std430) restrict readonly buffer VertIndices {
	uint b_vertIndices[];
};
layout(binding = 2, std430) restrict readonly buffer Vertices {
	float B_VERTICES[];
};
layout(binding = 3, std430) restrict readonly buffer Meshlets {
	Meshlet b_meshlets[];
};
layout(binding = 4, std430) restrict readonly buffer Instances {
	Instance b_instances[];
};
layout(binding = 5, std430) restrict readonly buffer Transforms {
	mat3x4 b_transforms[];
};

#include "../typesAccess.glsl"

void main() {
	
	Instance instance = b_instances[gl_VertexIndex >> 6];
	Meshlet meshlet = b_meshlets[instance.meshletIdx];
	
	uint triIdx = (gl_VertexIndex & bitmask(6)) + meshlet.vertexOffset;
	
	uint index = b_vertIndices[triIdx];
	vec3 vertex = fetchVertex(index);
	
	uint transformIdx = instance.objectIdx;
	mat4 transform = getTransform(b_transforms[transformIdx]);
	gl_Position = u_world.viewProjection * transform * vec4(vertex, 1.0);
	
}

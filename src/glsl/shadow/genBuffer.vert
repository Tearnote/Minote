#version 460
#pragma shader_stage(vertex)

#include "../types.glsl"

#define B_VERTICES b_vertices

layout(location = 0) out flat uint InstanceIdx;

layout(binding = 0, std430) restrict readonly buffer Vertices {
	float B_VERTICES[];
};
layout(binding = 1, std430) restrict readonly buffer Meshes {
	Mesh b_meshes[];
};
layout(binding = 2, std430) restrict readonly buffer Instances {
	Instance b_instances[];
};
layout(binding = 3, std430) restrict readonly buffer Transforms {
	mat3x4 b_transforms[];
};

layout(push_constant) uniform Constants {
	mat4 u_viewProjection;
};

#include "../typesAccess.glsl"

void main() {
	
	InstanceIdx = gl_InstanceIndex;
	
	vec3 vertex = fetchVertex(gl_VertexIndex);
	
	mat4 transform = b_meshes[b_instances[gl_InstanceIndex].meshIdx].transform;
	uint transformIdx = b_instances[gl_InstanceIndex].objectIdx;
	transform = getTransform(b_transforms[transformIdx]) * transform;
	gl_Position = u_viewProjection * transform * vec4(vertex, 1.0);
	
}

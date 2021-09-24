#version 460
#pragma shader_stage(vertex)

#include "types.glsl"

#define VERTICES_BUF b_vertices

layout(location = 0) out flat uint InstanceIndex;

layout(binding = 0) uniform WorldConstants {
	World u_world;
};
layout(std430, binding = 1) readonly buffer Vertices {
	float VERTICES_BUF[];
};
layout(std430, binding = 2) readonly buffer Transforms {
	Transform b_transforms[];
};

#include "typesAccess.glsl"

void main() {
	
	InstanceIndex = gl_InstanceIndex;
	
	vec3 vertex = fetchVertex(gl_VertexIndex);
	
	mat4 transform = getTransform(b_transforms[gl_InstanceIndex]);
	gl_Position = u_world.viewProjection * transform * vec4(vertex, 1.0);
	
}

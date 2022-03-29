#version 460
#pragma shader_stage(vertex)

#include "../types.glsl"

layout(binding = 0) uniform WorldConstants {
	World u_world;
};
layout(binding = 1, std430) restrict readonly buffer AABBs {
	float b_AABBs[];
};
layout(binding = 2, std430) restrict readonly buffer Instances {
	Instance b_instances[];
};
layout(binding = 3, std430) restrict readonly buffer Transforms {
	mat3x4 b_transforms[];
};

void main() {
	
	Instance instance = b_instances[gl_InstanceIndex];
	
	uint aabbOffset = instance.meshletIdx * 6;
	vec3 aabbMin = {
		b_AABBs[aabbOffset + 0],
		b_AABBs[aabbOffset + 1],
		b_AABBs[aabbOffset + 2]};
	vec3 aabbMax = {
		b_AABBs[aabbOffset + 3],
		b_AABBs[aabbOffset + 4],
		b_AABBs[aabbOffset + 5]};
	
	vec3 vertex;
	switch (gl_VertexIndex) {
	
	case  0: vertex = vec3(aabbMin.x, aabbMin.y, aabbMin.z); break;
	case  1: vertex = vec3(aabbMax.x, aabbMin.y, aabbMin.z); break;
	
	case  2: vertex = vec3(aabbMin.x, aabbMax.y, aabbMin.z); break;
	case  3: vertex = vec3(aabbMax.x, aabbMax.y, aabbMin.z); break;
	
	case  4: vertex = vec3(aabbMin.x, aabbMin.y, aabbMin.z); break;
	case  5: vertex = vec3(aabbMin.x, aabbMax.y, aabbMin.z); break;
	
	case  6: vertex = vec3(aabbMax.x, aabbMin.y, aabbMin.z); break;
	case  7: vertex = vec3(aabbMax.x, aabbMax.y, aabbMin.z); break;
	
	case  8: vertex = vec3(aabbMin.x, aabbMin.y, aabbMax.z); break;
	case  9: vertex = vec3(aabbMax.x, aabbMin.y, aabbMax.z); break;
	
	case 10: vertex = vec3(aabbMin.x, aabbMax.y, aabbMax.z); break;
	case 11: vertex = vec3(aabbMax.x, aabbMax.y, aabbMax.z); break;
	
	case 12: vertex = vec3(aabbMin.x, aabbMin.y, aabbMax.z); break;
	case 13: vertex = vec3(aabbMin.x, aabbMax.y, aabbMax.z); break;
	
	case 14: vertex = vec3(aabbMax.x, aabbMin.y, aabbMax.z); break;
	case 15: vertex = vec3(aabbMax.x, aabbMax.y, aabbMax.z); break;
	
	case 16: vertex = vec3(aabbMin.x, aabbMin.y, aabbMin.z); break;
	case 17: vertex = vec3(aabbMin.x, aabbMin.y, aabbMax.z); break;
	
	case 18: vertex = vec3(aabbMax.x, aabbMin.y, aabbMin.z); break;
	case 19: vertex = vec3(aabbMax.x, aabbMin.y, aabbMax.z); break;
	
	case 20: vertex = vec3(aabbMin.x, aabbMax.y, aabbMin.z); break;
	case 21: vertex = vec3(aabbMin.x, aabbMax.y, aabbMax.z); break;
	
	case 22: vertex = vec3(aabbMax.x, aabbMax.y, aabbMin.z); break;
	case 23: vertex = vec3(aabbMax.x, aabbMax.y, aabbMax.z); break;
	
	}
	
	uint transformIdx = instance.objectIdx;
	mat4 transform = getTransform(b_transforms[transformIdx]);
	
	gl_Position = u_world.viewProjection * transform * vec4(vertex, 1.0);
	
}

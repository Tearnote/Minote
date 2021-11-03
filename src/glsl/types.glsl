// Types commonly used for data interchange between rendering modules

#ifndef TYPES_GLSL
#define TYPES_GLSL

// Global world-data, constant throughout an entire frame
struct World {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	mat4 viewProjectionInverse;
	mat4 prevViewProjection;
	uvec2 viewportSize;
	vec2 pad0;
	vec3 cameraPos;
	uint frameCounter;
	
	vec3 sunDirection;
	float pad2;
	vec3 sunIlluminance;
};

// GPU representation of VkDrawIndexedIndirectCommand
struct Command {
	uint indexCount;
	uint instanceCount;
	uint firstIndex;
	uint vertexOffset;
	uint firstInstance;
};

// Index and vertex locations for drawing a specific mesh
struct MeshDescriptor {
	uint indexOffset;
	uint indexCount;
	uint vertexOffset;
	float radius; // Distance from origin to furthest vertex
};

struct Instance {
	uint meshIdx;
	uint materialIdx;
	uint colorIdx;
	uint transformIdx;
};

// Components of an instance transform
struct BasicTransform {
	vec3 position;
	float pad0;
	vec3 scale;
	float pad1;
	vec4 rotation; // wxyz quat
};

struct Material {
	uint id;
	float roughness;
	float metalness;
	uint pad0;
};

// Convert from BasicTransform to Transform
mat3x4 encodeTransform(BasicTransform _t) {
	
	const float rw = _t.rotation.x;
	const float rx = _t.rotation.y;
	const float ry = _t.rotation.z;
	const float rz = _t.rotation.w;
	
	mat3 rotationMat = mat3(
		1.0 - 2.0 * (ry * ry + rz * rz),       2.0 * (rx * ry - rw * rz),       2.0 * (rx * rz + rw * ry),
		      2.0 * (rx * ry + rw * rz), 1.0 - 2.0 * (rx * rx + rz * rz),       2.0 * (ry * rz - rw * rx),
		      2.0 * (rx * rz - rw * ry),       2.0 * (ry * rz + rw * rx), 1.0 - 2.0 * (rx * rx + ry * ry));
	
	rotationMat[0] *= _t.scale.x;
	rotationMat[1] *= _t.scale.y;
	rotationMat[2] *= _t.scale.z;
	
	mat3x4 result;
	result[0] = vec4(rotationMat[0], _t.position.x);
	result[1] = vec4(rotationMat[1], _t.position.y);
	result[2] = vec4(rotationMat[2], _t.position.z);
	return result;
	
}

// Convert from Transform to mat4 (standard column-major)
mat4 getTransform(mat3x4 _t) {
	
	return mat4(
		_t[0].x, _t[1].x, _t[2].x, 0.0,
		_t[0].y, _t[1].y, _t[2].y, 0.0,
		_t[0].z, _t[1].z, _t[2].z, 0.0,
		_t[0].w, _t[1].w, _t[2].w, 1.0);
	
}

#endif //TYPES_GLSL

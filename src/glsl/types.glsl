// Types commonly used for data interchange between rendering modules

#ifndef TYPES_GLSL
#define TYPES_GLSL

// Global world-data, constant throughout an entire frame
struct World {
	float4x4 view;
	float4x4 projection;
	float4x4 viewProjection;
	float4x4 viewProjectionInverse;
	float4x4 prevViewProjection;
	uint2 viewportSize;
	float nearPlane;
	float pad0;
	float3 cameraPos;
	uint frameCounter;
	
	float3 sunDirection;
	float pad1;
	float3 sunIlluminance;
};

// GPU representation of VkDrawIndexedIndirectCommand
struct Command {
	uint indexCount;
	uint instanceCount;
	uint firstIndex;
	uint vertexOffset;
	uint firstInstance;
};

// Index and vertex locations for drawing a specific model
struct Meshlet {
	uint materialIdx;
	
	uint indexOffset;
	uint indexCount;
	uint vertexOffset;
	
	float3 boundingSphereCenter;
	float boundingSphereRadius;
};

struct Model {
	uint meshletOffset;
	uint meshletCount;
};

struct Instance {
	uint objectIdx;
	uint meshletIdx;
};

// Components of an instance transform
struct BasicTransform {
	float3 position;
	float pad0;
	float3 scale;
	float pad1;
	float4 rotation; // wxyz quat
};

struct Material {
	float4 color;
	float3 emissive;
	uint id;
	float metalness;
	float roughness;
	float2 pad0;
};

// Convert from BasicTransform to Transform
float4x4 encodeTransform(BasicTransform _t) {
	
	const float rw = _t.rotation.x;
	const float rx = _t.rotation.y;
	const float ry = _t.rotation.z;
	const float rz = _t.rotation.w;
	
	float3x3 rotationMat = float3x3(
		1.0 - 2.0 * (ry * ry + rz * rz),       2.0 * (rx * ry - rw * rz),       2.0 * (rx * rz + rw * ry),
		      2.0 * (rx * ry + rw * rz), 1.0 - 2.0 * (rx * rx + rz * rz),       2.0 * (ry * rz - rw * rx),
		      2.0 * (rx * rz - rw * ry),       2.0 * (ry * rz + rw * rx), 1.0 - 2.0 * (rx * rx + ry * ry));
	
	rotationMat[0] *= _t.scale.x;
	rotationMat[1] *= _t.scale.y;
	rotationMat[2] *= _t.scale.z;
	
	rotationMat = transpose(rotationMat);
	
	float4x4 result;
	result[0] = float4(rotationMat[0], 0.0);
	result[1] = float4(rotationMat[1], 0.0);
	result[2] = float4(rotationMat[2], 0.0);
	result[3] = float4(_t.position, 1.0);
	return result;
	
}

// Convert from Transform to float4x4 (standard column-major)
float4x4 getTransform(float3x3x4 _t) {
	
	return float4x4(
		_t[0].x, _t[1].x, _t[2].x, 0.0,
		_t[0].y, _t[1].y, _t[2].y, 0.0,
		_t[0].z, _t[1].z, _t[2].z, 0.0,
		_t[0].w, _t[1].w, _t[2].w, 1.0);
	
}

#endif //TYPES_GLSL

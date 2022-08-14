#ifndef TYPES_HLSLI
#define TYPES_HLSLI

// VkDrawIndexedIndirectCommand
struct Command {
	uint indexCount;
	uint instanceCount;
	uint firstIndex;
	uint vertexOffset;
	uint firstInstance;
};

struct Material {
	float4 color;
	float3 emissive;
	uint id;
	float metalness;
	float roughness;
	float2 pad0;
};

struct Meshlet {
	uint indexOffset;
	uint indexCount;
	uint vertexOffset;
	
	uint materialIdx;
	
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

#endif

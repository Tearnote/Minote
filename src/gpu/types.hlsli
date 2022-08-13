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

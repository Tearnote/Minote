#ifndef WORLD_HLSL
#define WORLD_HLSL

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

#endif

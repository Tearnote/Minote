#include "instanceList/index.hlsli"
#include "types.hlsli"

[[vk::binding(0)]] StructuredBuffer<Meshlet> b_meshlets;
[[vk::binding(1)]] StructuredBuffer<uint> b_triIndices;
[[vk::binding(2)]] StructuredBuffer<uint4> b_instanceCount;
[[vk::binding(3)]] StructuredBuffer<Instance> b_instances;
[[vk::binding(4)]] RWStructuredBuffer<Command> b_command;
[[vk::binding(5)]] RWStructuredBuffer<uint> b_indices;

[[vk::constant_id(0)]] const uint MaxTrisPerMeshlet = 0;

[numthreads(64, 1, 1)]
void main(uint3 _tid: SV_DispatchThreadID) {
	
	// Retrieve instance details
	uint instanceIdx = _tid.x;
	if (instanceIdx >= b_instanceCount[0].w)
		return;
	Instance instance = b_instances[instanceIdx];
	Meshlet meshlet = b_meshlets[instance.meshletIdx];
	
	// Allocate output space
	uint triangleCount = meshlet.indexCount / 3;
	uint writeIdx;
	InterlockedAdd(b_command[0].indexCount, triangleCount * 3, writeIdx);
	
	// Write all meshlet indices
	for (uint i = 0; i < triangleCount; i += 1) {
		uint idx = i * 3;
		uint3 indices = {
			b_triIndices[meshlet.indexOffset + idx + 0],
			b_triIndices[meshlet.indexOffset + idx + 1],
			b_triIndices[meshlet.indexOffset + idx + 2],
		};
		indices = packIndex(indices, instanceIdx);
		b_indices[writeIdx + idx + 0] = indices[0];
		b_indices[writeIdx + idx + 1] = indices[1];
		b_indices[writeIdx + idx + 2] = indices[2];
	}
	
}

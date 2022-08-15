#include "instanceList/index.hlsli"
#include "visibility/worklist.hlsli"
#include "types.hlsli"

[[vk::binding(0)]] StructuredBuffer<Material> b_materials;
[[vk::binding(1)]] StructuredBuffer<Meshlet> b_meshlets;
[[vk::binding(2)]] StructuredBuffer<Instance> b_instances;
[[vk::binding(3)]] StructuredBuffer<uint> b_indices;
[[vk::binding(4)]] StructuredBuffer<uint> b_lists;
[[vk::binding(5)]] Texture2D<uint> s_visibility;
[[vk::binding(6)]] RWTexture2D<float4> t_target;

[[vk::constant_id(0)]] const uint TargetWidth = 0;
[[vk::constant_id(1)]] const uint TargetHeight = 0;
static const uint2 TargetSize = {TargetWidth, TargetHeight};
[[vk::constant_id(2)]] const uint TileOffset = 0;

[numthreads(1, TileSize, TileSize)]
void main(uint3 _tid: SV_DispatchThreadID) {
	
	_tid.xy = tiledPos(b_lists, _tid, TileOffset);
	if (any(_tid.xy >= TargetSize))
		return;
	
	// Retrieve instance details
	uint indexIdx = s_visibility[_tid.xy];
	if (indexIdx == -1u)
		return; // Sky pixel
	indexIdx *= 3; // Prim ID to index ID
	uint packedIndex = b_indices[indexIdx];
	uint index;
	uint instanceIdx;
	unpackIndex(packedIndex, index, instanceIdx);
	Instance instance = b_instances[instanceIdx];
	Meshlet meshlet = b_meshlets[instance.meshletIdx];
	Material material = b_materials[meshlet.materialIdx];
	if (material.id != MaterialFlat)
		return; // Not a flat shaded meshlet
	
	t_target[_tid.xy] = material.color + float4(material.emissive * 8.0, 0.0);
	
}

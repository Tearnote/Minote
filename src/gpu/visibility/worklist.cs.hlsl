#include "instanceList/index.hlsli"
#include "visibility/worklist.hlsli"
#include "types.hlsli"

[[vk::binding(0)]] StructuredBuffer<Meshlet> b_meshlets;
[[vk::binding(1)]] StructuredBuffer<Material> b_materials;
[[vk::binding(2)]] StructuredBuffer<Instance> b_instances;
[[vk::binding(3)]] StructuredBuffer<uint> b_indices;
[[vk::binding(4)]] Texture2D<uint> s_visibility;
[[vk::binding(5)]] RWStructuredBuffer<uint4> b_counts;
[[vk::binding(6)]] RWStructuredBuffer<uint> b_lists;

[[vk::constant_id(0)]] const uint VisibilityWidth = 0;
[[vk::constant_id(1)]] const uint VisibilityHeight = 0;
static const uint2 VisibilitySize = {VisibilityWidth, VisibilityHeight};
[[vk::constant_id(2)]] const uint TileAreaWidth = 0;
[[vk::constant_id(3)]] const uint TileAreaHeight = 0;
static const uint2 TileArea = {TileAreaWidth, TileAreaHeight};
[[vk::constant_id(4)]] const uint ListCount = 0;

groupshared uint sh_result;

[numthreads(TileSize, TileSize, 1)]
void main(uint3 _tid: SV_DispatchThreadID, uint3 _lid: SV_GroupThreadID, uint3 _gid: SV_GroupID) {
	
	if (all(_lid.xy == uint2(0, 0)))
		sh_result = 0;
	
	// Fetch material ID
	GroupMemoryBarrierWithGroupSync();
	uint materialID = -1u;
	if (all(_tid.xy < VisibilitySize)) {
		uint visValue = s_visibility[_tid.xy];
		if (visValue != -1u) {
			uint packedIndex = b_indices[visValue * 3];
			uint index;
			uint instanceIdx;
			unpackIndex(packedIndex, index, instanceIdx);
			uint meshletIdx = b_instances[instanceIdx].meshletIdx;
			uint materialIdx = b_meshlets[meshletIdx].materialIdx;
			materialID = b_materials[materialIdx].id;
		} else { // Sky
			materialID = 0;
		}
	}
	
	// Mark found material
	if (materialID != -1u)
		InterlockedOr(sh_result, 1u << materialID);
	
	// Write tiles
	GroupMemoryBarrierWithGroupSync();
	if (any(_lid.xy != uint2(0, 0)))
		return;
	
	uint tileCount = TileArea.x * TileArea.y;
	for (uint i = 0; i < ListCount; i += 1) {
		if ((sh_result & (1u << i)) == 0u)
			continue;
		
		uint tileOffset = tileCount * i;
		uint tileIdx;
		InterlockedAdd(b_counts[i].x, 1, tileIdx);
		uint tile = (_gid.x << 16) | _gid.y;
		b_lists[tileOffset + tileIdx] = tile;
	}
	
}

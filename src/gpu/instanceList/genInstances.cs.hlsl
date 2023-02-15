#include "types.hlsli"
#include "util.hlsli"

struct Constants {
	uint objectCount;
};

[[vk::binding(0)]] StructuredBuffer<Model> b_models;
[[vk::binding(1)]] StructuredBuffer<uint> b_modelIndices; // Index into b_models
[[vk::binding(2)]] RWStructuredBuffer<uint4> b_instanceCount;
[[vk::binding(3)]] RWStructuredBuffer<Instance> b_instances;

static const uint GroupSizeExp = 6; // Group size as 2^n
static const uint GroupSize = 1u << GroupSizeExp;

[[vk::push_constant]] Constants C;

groupshared uint sh_objectIndices[GroupSize];
groupshared uint sh_meshletCountPrefixSum[GroupSize];

// Fill out sh_objectIndices and sh_meshletCountPrefixSum
// sh_meshletCountPrefixSum[0] ends up bitpacked:
//   participating thread count in high bits,
//   thread group's total meshlet count in low bits
void meshletCountPrefixSum(uint _objectIdx) {
	
	uint meshletCount = b_models[b_modelIndices[_objectIdx]].meshletCount;
	const uint HighOne = 1u << (32u - GroupSizeExp); // We put a 1 in high bits to track atomic execution order
	uint accumAdd = HighOne | meshletCount;
	uint accum;
	InterlockedAdd(sh_meshletCountPrefixSum[0], accumAdd, accum);
	uint atomicIdx = accum >> (32u - GroupSizeExp);
	uint prefixSum = accum & bitmask(32u - GroupSizeExp);
	// Write out prefix summed and shuffled data
	sh_objectIndices[atomicIdx] = _objectIdx;
	if (atomicIdx != 0)
		sh_meshletCountPrefixSum[atomicIdx] = prefixSum;
	
}

// Find the largest index containing a value smaller than target
uint binarySearchSharedIdx(uint _target){
	
	uint result = GroupSize / 2;
	for (uint i = 0; i < GroupSizeExp - 1; i += 1) {
		uint searchStride = 1u << (GroupSizeExp - 2u - i);
		uint value;
		if (result == 0)
			value = 0;
		else
			value = sh_meshletCountPrefixSum[result];
		if (_target >= value)
			result += searchStride;
		else
			result -= searchStride;
	}
	// Final step
	{
		uint value;
		if (result == 0)
			value = 0;
		else
			value = sh_meshletCountPrefixSum[result];
		if (_target < value)
			result -= 1;
	}
	return result;
	
}

// Write _count meshlets into output at _outputIdx, starting from _meshletIdx of object at _sharedIdx
void writeOutFromOffset(uint _sharedIdx, uint _meshletIdx, uint _count, uint _outputIdx) {
	
	// Cached values
	uint objectIdx = sh_objectIndices[_sharedIdx];
	Model model = b_models[b_modelIndices[objectIdx]];
	
	for (uint i = 0; i < _count; i += 1) {
		Instance output;
		output.objectIdx = objectIdx;
		output.meshletIdx = model.meshletOffset + _meshletIdx;
		b_instances[_outputIdx + i] = output;
		
		// Advance state
		_meshletIdx += 1;
		// End of object; move to the next and update cache
		if (_meshletIdx >= model.meshletCount){
			_sharedIdx += 1;
			objectIdx = sh_objectIndices[_sharedIdx];
			model = b_models[b_modelIndices[objectIdx]];
			_meshletIdx = 0;
		}
	}
	
}

[numthreads(GroupSize, 1, 1)]
void main(uint3 _tid: SV_DispatchThreadID, uint3 _lid: SV_GroupThreadID) {
	
	// Index 0 is used as a counter; the rest should be max uint
	// to not participate in binary search
	if (_lid.x == 0)
		sh_meshletCountPrefixSum[_lid.x] = 0;
	else
		sh_meshletCountPrefixSum[_lid.x] = -1;
	
	uint objectIdx;
	if (_tid.x < C.objectCount)
		objectIdx = _tid.x;
	else
		objectIdx = -1;
	
	GroupMemoryBarrierWithGroupSync();
	if (objectIdx != -1) // Run prefix sum with as many threads as objects
		meshletCountPrefixSum(objectIdx);
	
	GroupMemoryBarrierWithGroupSync();
	// Find starting point for this thread
	const uint AccumLowBits = bitmask(32u - GroupSizeExp);
	uint totalMeshletCount = sh_meshletCountPrefixSum[0] & AccumLowBits;
	uint meshletStride = divRoundUp(totalMeshletCount, GroupSize);
	uint startingMeshletIdx = meshletStride * _lid.x;
	uint startingSharedIdx = binarySearchSharedIdx(startingMeshletIdx);
	
	// Allocate space for output instances
	if(startingMeshletIdx >= totalMeshletCount)
		return; // Starts past the buffer; useless thread
	uint threadWorkload = min(meshletStride, totalMeshletCount - startingMeshletIdx);
	uint writeIndex;
	InterlockedAdd(b_instanceCount[0].w, threadWorkload, writeIndex);
		
	uint groupCount = divRoundUp(writeIndex + threadWorkload, 64u) - divRoundUp(writeIndex, 64u);
	if (groupCount > 0)
		InterlockedAdd(b_instanceCount[0].x, groupCount);
	
	// Write out assigned workload
	if (startingSharedIdx != 0)
		startingMeshletIdx -= sh_meshletCountPrefixSum[startingSharedIdx]; // From workload offset to object offset
	writeOutFromOffset(startingSharedIdx, startingMeshletIdx, threadWorkload, writeIndex);
	
}
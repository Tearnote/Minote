#ifndef INSTANCELIST_INDICES_HLSLI
#define INSTANCELIST_INDICES_HLSLI

#include "util.hlsli"

static const uint IndexInstanceBitOffset = 6;

template<typename T>
T packIndex(T _index, uint _instance) {
	
	return _index | (_instance << IndexInstanceBitOffset);
	
}

void unpackIndex(uint _packedIndex, out uint _index, out uint _instance) {
	
	_index = _packedIndex & bitmask(IndexInstanceBitOffset);
	_instance = _packedIndex >> IndexInstanceBitOffset;
	
}

#endif

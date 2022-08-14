#ifndef VISIBILITY_WORKLIST_HLSLI
#define VISIBILITY_WORKLIST_HLSLI

#include "util.hlsli"

static const uint TileSize = 8;

// This function expects _tid.x to have the tile idx, and _tid.yz are [0, TileSize)
uint2 tiledPos(StructuredBuffer<uint> _buf, uint3 _tid, uint _offset) {
	
	uint tilePacked = _buf[_tid.x + _offset];
	return uint2(tilePacked >> 16, tilePacked & bitmask(16)) * TileSize + _tid.yz;
	
}

#endif

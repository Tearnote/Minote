#ifndef UTIL_HLSLI
#define UTIL_HLSLI

template<typename T>
T divRoundUp(T _n, T _div) {
	
	return (_n + (_div - 1)) / _div;
	
}

uint bitmask(uint _bits) {
	
	if (_bits == 32)
		return -1u;
	else
		return (1u << _bits) - 1u;
	
}

// Emulating the missing subgroupClusteredmin() intrinsic from GLSL
template<typename T>
T WaveClusteredMin(T _val, uint _clusterSize) {
	
	T result;
	uint loops = WaveGetLaneCount() / _clusterSize;
	for (uint i = 0; i < loops; i += 1) {
		if (WaveGetLaneIndex() / _clusterSize == i)
			result = WaveActiveMin(_val);
	}
	return result;
	
}

// https://fgiesen.wordpress.com/2009/12/13/decoding-morton-codes/
uint mortonCompact1By1(uint _x) {
	
	_x &= 0x55555555u;                    // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
	_x = (_x ^ (_x >>  1)) & 0x33333333u; // x = --fe --dc --ba --98 --76 --54 --32 --10
	_x = (_x ^ (_x >>  2)) & 0x0f0f0f0fu; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
	_x = (_x ^ (_x >>  4)) & 0x00ff00ffu; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
	_x = (_x ^ (_x >>  8)) & 0x0000ffffu; // x = ---- ---- ---- ---- fedc ba98 7654 3210
	return _x;
	
}

// Get thread idx using morton coding for optimal access and wave intrinsics usage
// Expected _tid:
// x - invocation within group, [0,tileSize*tileSize)
// yz - xy index of the group's tile, each tile is a square with tileSize length of each edge
uint2 mortonOrder(uint3 _tid, uint2 _tileSize) {
	
	uint2 localID = {mortonCompact1By1(_tid.x), mortonCompact1By1(_tid.x >> 1)};
	uint2 tileOffset = _tid.yz * _tileSize;
	return localID + tileOffset;
	
}

#endif

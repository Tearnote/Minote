#ifndef WORKLIST_GLSL
#define WORKLIST_GLSL

#include "../util.glsl"

#define WORKLIST_TILESIZE 8

#ifdef B_LISTS

uint2 getTileGid(uint _tileIdx, uint _tileOffset) {
	
	uint tilePacked = B_LISTS[_tileOffset + _tileIdx];
	uint2 tile = uint16Fromuint(tilePacked);
	return tile * WORKLIST_TILESIZE;
	
}

#endif //B_LISTS

#endif //WORKLIST_GLSL

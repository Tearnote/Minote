#ifndef TYPESACCESS_GLSL
#define TYPESACCESS_GLSL

#include "types.glsl"
#include "util.glsl"

#ifdef B_INDICES

// Unpack and return indices of a given triangle.
uint3 fetchIndices(uint _n) {
	
	return uint3(
		B_INDICES[_n + 0],
		B_INDICES[_n + 1],
		B_INDICES[_n + 2]);
	
}

#endif //B_INDICES

#ifdef B_VERTICES

float3 fetchVertex(uint _n) {
	
	uint base = _n * 3;
	
	return float3(
		B_VERTICES[base + 0],
		B_VERTICES[base + 1],
		B_VERTICES[base + 2]);
	
}

#endif //B_VERTICES

#ifdef B_NORMALS

float3 fetchNormal(uint _n) {
	
	uint octNormal = B_NORMALS[_n];
	return octDecode(octNormal);
	
}

#endif //B_NORMALS

#ifdef B_COLORS

float4 fetchColor(uint _n) {
	
	uint colorPacked = B_COLORS[_n];
	uint4 uresult = {
		(colorPacked >>  0) & bitmask(8),
		(colorPacked >>  8) & bitmask(8),
		(colorPacked >> 16) & bitmask(8),
		(colorPacked >> 24) };
	float4 result = float4(uresult);
	result /= float(0xFF);
	result.rgb = pow(result.rgb, float3(2.2));
	
	return result;
	
}

#endif //B_COLORS

#endif //TYPESACCESS_GLSL

#ifndef TYPESACCESS_GLSL
#define TYPESACCESS_GLSL

#include "types.glsl"
#include "util.glsl"

#ifdef B_INDICES

// Unpack and return indices of a given triangle.
uvec3 fetchIndices(uint _n) {
	
	return uvec3(
		B_INDICES[_n + 0],
		B_INDICES[_n + 1],
		B_INDICES[_n + 2]);
	
}

#endif //B_INDICES

#ifdef B_VERTICES

vec3 fetchVertex(uint _n) {
	
	uint base = _n * 3;
	
	return vec3(
		B_VERTICES[base + 0],
		B_VERTICES[base + 1],
		B_VERTICES[base + 2]);
	
}

#endif //B_VERTICES

#ifdef B_NORMALS

vec3 fetchNormal(uint _n) {
	
	uint base = _n * 3;
	
	return vec3(
		B_NORMALS[base + 0],
		B_NORMALS[base + 1],
		B_NORMALS[base + 2]);
	
}

#endif //B_NORMALS

#ifdef B_COLORS

vec4 fetchColor(uint _n) {
	
	uint base = _n * 2;
	
	uvec2 fetches = {
		B_COLORS[base + 0],
		B_COLORS[base + 1]};
	uvec4 uresult = {
		u32Lower(fetches.x),
		u32Upper(fetches.x),
		u32Lower(fetches.y),
		u32Upper(fetches.y),
	};
	vec4 result = vec4(uresult);
	result /= float(bitmask(16));
	
	return result;
	
}

#endif //B_COLORS

#endif //TYPESACCESS_GLSL

#ifndef TYPESACCESS_GLSL
#define TYPESACCESS_GLSL

#include "types.glsl"
#include "util.glsl"

#ifdef B_INDICES

// Unpack and return indices of a given triangle.
uvec3 fetchIndices(uint _n) {
	
	uint offset = _n / 2u;
	uvec2 fetches = {
	B_INDICES[offset + 0],
	B_INDICES[offset + 1]};
	
	if ((_n & 1u) == 0u)
		return uvec3(
			u32Lower(fetches.x),
			u32Upper(fetches.x),
			u32Lower(fetches.y));
	else
		return uvec3(
			u32Upper(fetches.x),
			u32Lower(fetches.y),
			u32Upper(fetches.y));
	
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
	
	uvec2 fetches = {
		B_COLORS[_n * 2 + 0],
		B_COLORS[_n * 2 + 1]};
	uvec4 uresult = {
		u32Lower(fetches.x),
		u32Upper(fetches.x),
		u32Lower(fetches.y),
		u32Upper(fetches.y),
	};
	vec4 result = vec4(uresult);
	result /= float((1u << 16u) - 1u);
	
	return result;
	
}

#endif //B_COLORS

#endif //TYPESACCESS_GLSL

#ifndef TYPESACCESS_GLSL
#define TYPESACCESS_GLSL

#include "types.glsl"
#include "util.glsl"

#ifdef INDICES_BUF

// Unpack and return indices of a given triangle.
uvec3 fetchIndices(uint _n) {
	
	uint offset = _n / 2u;
	uvec2 fetches = {
	INDICES_BUF[offset + 0],
	INDICES_BUF[offset + 1]};
	
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

#endif //INDICES_BUF

#ifdef VERTICES_BUF

vec3 fetchVertex(uint _n) {
	
	uint base = _n * 3;
	
	return vec3(
		VERTICES_BUF[base + 0],
		VERTICES_BUF[base + 1],
		VERTICES_BUF[base + 2]);
	
}

#endif //VERTICES_BUF

#ifdef NORMALS_BUF

vec3 fetchNormal(uint _n) {
	
	uint base = _n * 3;
	
	return vec3(
		NORMALS_BUF[base + 0],
		NORMALS_BUF[base + 1],
		NORMALS_BUF[base + 2]);
	
}

#endif //NORMALS_BUF

#ifdef COLORS_BUF

vec4 fetchColor(uint _n) {
	
	uvec2 fetches = {
		COLORS_BUF[_n * 2 + 0],
		COLORS_BUF[_n * 2 + 1]};
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

#endif //COLORS_BUF

#endif //TYPESACCESS_GLSL

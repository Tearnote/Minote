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

const uint NormalOctWidth = 16;

vec3 octDecode(uint _oct) {
	
	uint mu = (1u << NormalOctWidth) - 1u;
	
	uvec2 d = uvec2(_oct, _oct >> NormalOctWidth) & mu;
	vec2 v = vec2(d) / float(mu);
	
	v = v * 2.0 - 1.0;
	vec3 norm = vec3(v, 1.0 - abs(v.x) - abs(v.y));
	float t = max(-norm.z, 0.0);
	norm.x += (norm.x > 0.0)? -t : t;
	norm.y += (norm.y > 0.0)? -t : t;
	
	return normalize(norm);
	
}

vec3 fetchNormal(uint _n) {
	
	uint octNormal = B_NORMALS[_n];
	return octDecode(octNormal);
	
}

#endif //B_NORMALS

#ifdef B_COLORS

vec4 fetchColor(uint _n) {
	
	uint colorPacked = B_COLORS[_n];
	uvec4 uresult = {
		(colorPacked >>  0) & bitmask(8),
		(colorPacked >>  8) & bitmask(8),
		(colorPacked >> 16) & bitmask(8),
		(colorPacked >> 24) };
	vec4 result = vec4(uresult);
	result /= float(0xFF);
	result.rgb = pow(result.rgb, vec3(2.2));
	
	return result;
	
}

#endif //B_COLORS

#endif //TYPESACCESS_GLSL

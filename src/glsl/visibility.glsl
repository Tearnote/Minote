// Utilities for working with visibility buffer data

#ifndef VISIBILITY_GLSL
#define VISIBILITY_GLSL

#include "visibilityTypes.glsl"
#include "util.glsl"

#define TRIANGLE_ID_BITS 20u

VisSample unpackVisibility(uint _packed) {
	
	VisSample result = VisSample(
		_packed >> TRIANGLE_ID_BITS,
		_packed & bitmask(TRIANGLE_ID_BITS));
	return result;
	
}

// https://github.com/GraphicsProgramming/RVPT/blob/master/assets/shaders/intersection.glsl
// Used under Apache 2.0 license - see licenses directory
void lineTriIntersection(out vec3 _position, out vec3 _barycentrics,
	vec3 _lineOrigin, vec3 _lineDir, mat3 _triangle) {
	
	vec3 e0 = _triangle[1] - _triangle[0];
	vec3 e1 = _triangle[2] - _triangle[0];
	vec3 n = cross(e0,e1);
	
	float t = dot(_triangle[0] - _lineOrigin, n) / dot(_lineDir, n);
	vec3 p = _lineOrigin + t * _lineDir;
	
	vec3 p0 = p - _triangle[0];
	vec2 b = vec2(dot(p0, e0), dot(p0, e1));
	mat2 A_adj = mat2(dot(e1, e1), -dot(e0, e1), -dot(e0, e1), dot(e0, e0));
	float inv_det = 1.0 / (A_adj[0][0] * A_adj[1][1] - A_adj[0][1] * A_adj[1][0]);
	vec2 uv = inv_det * (A_adj * b);
	
	_barycentrics = vec3(1.0 - uv.x - uv.y, uv.x, uv.y);
	_position = p;
	
}

// https://github.com/GraphicsProgramming/RVPT/blob/master/assets/shaders/intersection.glsl
// Used under Apache 2.0 license - see licenses directory
bool lineTriIntersectionDist(/*out vec3 _position, out vec3 _barycentrics,*/ out float _distance,
	vec3 _lineOrigin, vec3 _lineDir, mat3 _triangle) {
	
	vec3 e0 = _triangle[1] - _triangle[0];
	vec3 e1 = _triangle[2] - _triangle[0];
	vec3 n = cross(e0,e1);
	
	float t = dot(_triangle[0] - _lineOrigin, n) / dot(_lineDir, n);
	vec3 p = _lineOrigin + t * _lineDir;
	
	vec3 p0 = p - _triangle[0];
	vec2 b = vec2(dot(p0, e0), dot(p0, e1));
	mat2 A_adj = mat2(dot(e1, e1), -dot(e0, e1), -dot(e0, e1), dot(e0, e0));
	float inv_det = 1.0 / (A_adj[0][0] * A_adj[1][1] - A_adj[0][1] * A_adj[1][0]);
	vec2 uv = inv_det * (A_adj * b);
	
	// _barycentrics = vec3(1.0 - uv.x - uv.y, uv.x, uv.y);
	// _position = p;
	_distance = t;
	return uv.x >= 0.0 && uv.y >= 0.0 && uv.x + uv.y <= 1.0;
	
}

// Clamp barycentrics between 0.0 and 1.0 so that they're inside of a triangle,
// and renormalize.
vec3 clampBarycentrics(vec3 _barycentrics) {
	
	_barycentrics = clamp(_barycentrics, vec3(0.0), vec3(1.0));
	return _barycentrics / dot(_barycentrics, vec3(1.0));
	
}

// 3-way slerp, for normal interpolation.
// http://math.ucsd.edu/~sbuss/ResearchWeb/spheremean/

vec3 normalInterpProject(vec3 _u, vec3 _v) {
	
	vec3 result = _u - _v;
	return result - (dot(result, _v) * _v);
	
}

vec3 normalInterpRotateUnitInDirection(vec3 _u, vec3 _dir) {
	
	float theta = dot(_dir, _dir);
	if (theta == 0.0)
		return _u;
	
	theta = sqrt(theta);
	float cosTheta = cos(theta);
	float sinTheta = sin(theta);
	vec3 dirUnit = _dir / theta;
	return cosTheta * _u + sinTheta * dirUnit;
	
}

vec3 normalInterpEstimate(vec3 _estimate, mat3 _normals, vec3 _weights) {
	
	vec3 basisX, basisY;
	orthonormalBasis(_estimate, basisX, basisY);
	
	mat2 hessian = mat2(0.0);
	vec2 gradient = vec2(0.0);
	
	for (uint i = 0; i < 3; i += 1) {
		
		vec3 perp = normalInterpProject(_normals[i], _estimate);
		float sinTheta = length(perp);
		
		if (abs(sinTheta) < 0.00001) {
			
			hessian += mat2(_weights[i], 0.0, 0.0, _weights[i]);
			continue;
			
		}
		
		float cosTheta = dot(_normals[i], _estimate);
		float theta = fastAtan(sinTheta, cosTheta);
		float sinThetaInv = 1.0 / sinTheta;
		vec3 perpNorm = perp * sinThetaInv; // Normalize
		float cosPhi = dot(perpNorm, basisX);
		float sinPhi = dot(perpNorm, basisY);
		vec2 gradLocal = {cosPhi, sinPhi};
		gradient += _weights[i] * theta * gradLocal; // Added on weighted discrepancy to gradient
		float sinPhiSq = sinPhi * sinPhi;
		float cosPhiSq = cosPhi * cosPhi;
		float tt = _weights[i] * theta * sinThetaInv * cosTheta;
		float offdiag = cosPhi * sinPhi * (_weights[i] - tt);
		hessian += mat2(
		_weights[i] * cosPhiSq + tt * sinPhiSq,
		offdiag, offdiag,
		_weights[i] * sinPhiSq + tt * cosPhiSq);
		
	}
	
	vec2 dispLocal = inverse(hessian) * gradient;
	vec3 disp = dispLocal.x * basisX + dispLocal.y * basisY;
	
	return normalInterpRotateUnitInDirection(_estimate, disp);
	
}

vec3 normalInterp(mat3 _normals, vec3 _weights) {
	
	vec3 estimate = normalize(
	_normals[0] * _weights.x +
	_normals[1] * _weights.y +
	_normals[2] * _weights.z);
	
	for (uint i = 0; i < 1; i += 1) // Just one round seems to be a very strong approximation
	estimate = normalInterpEstimate(estimate, _normals, _weights);
	
	return estimate;
	
}

#endif //VISIBILITY_GLSL

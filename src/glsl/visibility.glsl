#ifndef VISIBILITY_GLSL
#define VISIBILITY_GLSL

#include "util.glsl"

// Utilities for working with visibility buffer data

// Barycentrics interpolator. The below 3 functions are made available under
// the following terms. The full Apache 2.0 license is included with the source.
// The source has been modified to comply with project code style.
/*
Copyright (C) 2019 - DevSH Graphics Programming Sp. z O.O.
This software is provided 'as-is', under the Apache 2.0 license,
without any express or implied warranty.  In no event will the authors
be held liable for any damages arising from the use of this software.
*/
vec2 reconstructBarycentrics(vec3 _positionRelativeToV0, mat2x3 _edges) {
	
	float e0_2 = dot(_edges[0], _edges[0]);
	float e0e1 = dot(_edges[0], _edges[1]);
	float e1_2 = dot(_edges[1], _edges[1]);
	
	float qe0 = dot(_positionRelativeToV0, _edges[0]);
	float qe1 = dot(_positionRelativeToV0, _edges[1]);
	vec2 protoBary = vec2(qe0*e1_2 - qe1*e0e1, qe1*e0_2 - qe0*e0e1);
	
	float rcp_dep = 1.0 / (e0_2*e1_2 - e0e1*e0e1);
	return protoBary * rcp_dep;
	
}

vec2 reconstructBarycentrics(vec3 _pointPosition, mat3 _vertexPositions) {
	
	return reconstructBarycentrics(
		_pointPosition - _vertexPositions[2],
		mat2x3(_vertexPositions[0] - _vertexPositions[2], _vertexPositions[1] - _vertexPositions[2]));
	
}

vec3 expandBarycentrics(vec2 _compactBarycentrics) {
	
	return vec3(_compactBarycentrics.xy, 1.0 - _compactBarycentrics.x - _compactBarycentrics.y);
	
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
		float theta = atan(sinTheta, cosTheta);
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

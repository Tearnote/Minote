// Utilities for working with visibility buffer data

#ifndef VISBUF_GLSL
#define VISBUF_GLSL

#include "visbufTypes.glsl"
#include "../util.glsl"

float3 calculateBarycentrics(float3x3x4 _verts, float2 _pxNdc) {
	
	float3 invW = rcp(float3(_verts[0].w, _verts[1].w, _verts[2].w));
	
	float2 ndc0 = _verts[0].xy * invW.x;
	float2 ndc1 = _verts[1].xy * invW.y;
	float2 ndc2 = _verts[2].xy * invW.z;
	
	float invDet = rcp(determinant(mat2(ndc2 - ndc1, ndc0 - ndc1)));
	float3 m_ddx = float3(ndc1.y - ndc2.y, ndc2.y - ndc0.y, ndc0.y - ndc1.y) * invDet;
	float3 m_ddy = float3(ndc2.x - ndc1.x, ndc0.x - ndc2.x, ndc1.x - ndc0.x) * invDet;
	
	float2 deltaVec = _pxNdc - ndc0;
	float interpInvW = (invW.x + deltaVec.x * dot(invW, m_ddx) + deltaVec.y * dot(invW, m_ddy));
	float interpW = rcp(interpInvW);
	
	return float3(
		interpW * (invW[0] + deltaVec.x * m_ddx.x * invW[0] + deltaVec.y * m_ddy.x * invW[0]),
		interpW * (          deltaVec.x * m_ddx.y * invW[1] + deltaVec.y * m_ddy.y * invW[1]),
		interpW * (          deltaVec.x * m_ddx.z * invW[2] + deltaVec.y * m_ddy.z * invW[2]) );
	
}

// 3-way slerp, for normal interpolation.
// http://math.ucsd.edu/~sbuss/ResearchWeb/spheremean/

float3 normalInterpProject(float3 _u, float3 _v) {
	
	float3 result = _u - _v;
	return result - (dot(result, _v) * _v);
	
}

float3 normalInterpRotateUnitInDirection(float3 _u, float3 _dir) {
	
	float theta = dot(_dir, _dir);
	if (theta == 0.0)
		return _u;
	
	theta = sqrt(theta);
	float cosTheta = cos(theta);
	float sinTheta = sin(theta);
	float3 dirUnit = _dir / theta;
	return cosTheta * _u + sinTheta * dirUnit;
	
}

float3 normalInterpEstimate(float3 _estimate, float3x3 _normals, float3 _weights) {
	
	float3 basisX, basisY;
	orthonormalBasis(_estimate, basisX, basisY);
	
	mat2 hessian = mat2(0.0);
	float2 gradient = float2(0.0);
	
	for (uint i = 0; i < 3; i += 1) {
		
		float3 perp = normalInterpProject(_normals[i], _estimate);
		float sinTheta = length(perp);
		
		if (abs(sinTheta) < 0.00001) {
			
			hessian += mat2(_weights[i], 0.0, 0.0, _weights[i]);
			continue;
			
		}
		
		float cosTheta = dot(_normals[i], _estimate);
		float theta = fastAtan(sinTheta, cosTheta);
		float sinThetaInv = 1.0 / sinTheta;
		float3 perpNorm = perp * sinThetaInv; // Normalize
		float cosPhi = dot(perpNorm, basisX);
		float sinPhi = dot(perpNorm, basisY);
		float2 gradLocal = {cosPhi, sinPhi};
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
	
	float2 dispLocal = inverse(hessian) * gradient;
	float3 disp = dispLocal.x * basisX + dispLocal.y * basisY;
	
	return normalInterpRotateUnitInDirection(_estimate, disp);
	
}

float3 normalInterp(float3x3 _normals, float3 _weights) {
	
	float3 estimate = normalize(
	_normals[0] * _weights.x +
	_normals[1] * _weights.y +
	_normals[2] * _weights.z);
	
	for (uint i = 0; i < 1; i += 1) // Just one round seems to be a very strong approximation
	estimate = normalInterpEstimate(estimate, _normals, _weights);
	
	return estimate;
	
}

#endif //VISBUF_GLSL

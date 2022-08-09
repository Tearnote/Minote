#ifndef SKY_ACCESS_HLSLI
#define SKY_ACCESS_HLSLI

#include "sky/types.hlsli"
#include "constants.hlsli"

#define NONLINEARSKYVIEWLUT 1

void uvToLutTransmittanceParams(out float _viewHeight, out float _viewZenithCosAngle,
	AtmosphereParams _params, float2 _uv) {
	
	float x_mu = _uv.x;
	float x_r = _uv.y;
	
	float H = sqrt(_params.topRadius * _params.topRadius - _params.bottomRadius * _params.bottomRadius);
	float rho = H * x_r;
	_viewHeight = sqrt(rho * rho + _params.bottomRadius * _params.bottomRadius);
	
	float d_min = _params.topRadius - _viewHeight;
	float d_max = rho + H;
	float d = d_min + x_mu * (d_max - d_min);
	_viewZenithCosAngle = d == 0.0? 1.0 : (H * H - rho * rho - d * d) / (2.0 * _viewHeight * d);
	_viewZenithCosAngle = clamp(_viewZenithCosAngle, -1.0, 1.0);
	
}

void lutTransmittanceParamsToUv(out float2 _uv,
	AtmosphereParams _params, float _viewHeight, float _viewZenithCosAngle) {
	
	float H = sqrt(max(0.0, _params.topRadius * _params.topRadius - _params.bottomRadius * _params.bottomRadius));
	float rho = sqrt(max(0.0, _viewHeight * _viewHeight - _params.bottomRadius * _params.bottomRadius));
	
	float discriminant = _viewHeight * _viewHeight * (_viewZenithCosAngle * _viewZenithCosAngle - 1.0) +
		_params.topRadius * _params.topRadius;
	float d = max(0.0, (-_viewHeight * _viewZenithCosAngle + sqrt(discriminant))); // Distance to atmosphere boundary
	
	float d_min = _params.topRadius - _viewHeight;
	float d_max = rho + H;
	float x_mu = (d - d_min) / (d_max - d_min);
	float x_r = rho / H;
	
	_uv = float2(x_mu, x_r);
	
}

float fromSubUvsToUnit(float _u, float _resolution) {
	
	return (_u - 0.5 / _resolution) * (_resolution / (_resolution - 1.0));
	
}

float fromUnitToSubUvs(float _u, float _resolution) {
	
	return (_u + 0.5 / _resolution) * (_resolution / (_resolution + 1.0));
	
}

void uvToSkyViewLutParams(out float _viewZenithCosAngle, out float _lightViewCosAngle,
	AtmosphereParams _params, uint2 _viewSize, float _viewHeight, float2 _uv) {
	
	// Constrain uvs to valid sub texel range (avoid zenith derivative issue making LUT usage visible)
	_uv = float2(fromSubUvsToUnit(_uv.x, _viewSize.x), fromSubUvsToUnit(_uv.y, _viewSize.y));
	
	float vHorizon = sqrt(_viewHeight * _viewHeight - _params.bottomRadius * _params.bottomRadius);
	float cosBeta = vHorizon / _viewHeight; // GroundToHorizonCos
	float beta = acos(cosBeta);
	float zenithHorizonAngle = PI - beta;
	
	if (_uv.y < 0.5) {
		float coord = 2.0 * _uv.y;
#if NONLINEARSKYVIEWLUT
		coord = 1.0 - coord;
		coord *= coord;
		coord = 1.0 - coord;
#endif
		_viewZenithCosAngle = cos(zenithHorizonAngle * coord);
	} else {
		float coord = _uv.y * 2.0 - 1.0;
#if NONLINEARSKYVIEWLUT
		coord *= coord;
#endif
		_viewZenithCosAngle = cos(zenithHorizonAngle + beta * coord);
	}
	
	float coord = _uv.x;
	coord *= coord;
	_lightViewCosAngle = -(coord * 2.0 - 1.0);
	
}

void skyViewLutParamsToUv(out float2 _uv, bool _intersectGround, float _viewZenithCosAngle,
	float _lightViewCosAngle, uint2 _viewSize, float _viewHeight, float _atmoBottom) {
		
	float vHorizon = sqrt(_viewHeight * _viewHeight - _atmoBottom * _atmoBottom);
	float cosBeta = vHorizon / _viewHeight; // GroundToHorizonCos
	float beta = acos(cosBeta);
	float zenithHorizonAngle = PI - beta;

	if (!_intersectGround) {
		float coord = acos(_viewZenithCosAngle) / zenithHorizonAngle;
#if NONLINEARSKYVIEWLUT
		coord = 1.0 - coord;
		coord = sqrt(coord);
		coord = 1.0 - coord;
#endif //NONLINEARSKYVIEWLUT
		_uv.y = coord * 0.5;
	} else {
		float coord = (acos(_viewZenithCosAngle) - zenithHorizonAngle) / beta;
#if NONLINEARSKYVIEWLUT
		coord = sqrt(coord);
#endif //NONLINEARSKYVIEWLUT
		_uv.y = coord * 0.5 + 0.5;
	}
	
	float coord = -_lightViewCosAngle * 0.5 + 0.5;
	coord = sqrt(coord);
	_uv.x = coord;
	
	// Constrain uvs to valid sub texel range (avoid zenith derivative issue making LUT usage visible)
	_uv = float2(fromUnitToSubUvs(_uv.x, _viewSize.x), fromUnitToSubUvs(_uv.y, _viewSize.y));
	
}

#endif

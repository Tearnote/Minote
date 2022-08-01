#ifndef SKY_ACCESS_HLSL
#define SKY_ACCESS_HLSL

#include "types.hlsl"

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

#endif

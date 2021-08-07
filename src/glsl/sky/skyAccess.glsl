// Helpers for addressing the LOD textures

#include "../constants.glsl"

#define NONLINEARSKYVIEWLUT 1
#define AP_KM_PER_SLICE 4.0

float fromUnitToSubUvs(float _u, float _resolution) {
	
	return (_u + 0.5 / _resolution) * (_resolution / (_resolution + 1.0));
	
}

float fromSubUvsToUnit(float _u, float _resolution) {
	
	return (_u - 0.5 / _resolution) * (_resolution / (_resolution - 1.0));
	
}

void uvToLutTransmittanceParams(out float _viewHeight, out float _viewZenithCosAngle,
	vec2 _uv, float _atmoBottom, float _atmoTop) {
	
	float x_mu = _uv.x;
	float x_r = _uv.y;

	float H = sqrt(_atmoTop * _atmoTop - _atmoBottom * _atmoBottom);
	float rho = H * x_r;
	_viewHeight = sqrt(rho * rho + _atmoBottom * _atmoBottom);

	float d_min = _atmoTop - _viewHeight;
	float d_max = rho + H;
	float d = d_min + x_mu * (d_max - d_min);
	_viewZenithCosAngle = d == 0.0 ? 1.0 : (H * H - rho * rho - d * d) / (2.0 * _viewHeight * d);
	_viewZenithCosAngle = clamp(_viewZenithCosAngle, -1.0, 1.0);
	
}

void lutTransmittanceParamsToUv(float _viewHeight, float _viewZenithCosAngle,
	out vec2 _uv, float _atmoBottom, float _atmoTop) {
	
	float H = sqrt(max(0.0, _atmoTop * _atmoTop - _atmoBottom * _atmoBottom));
	float rho = sqrt(max(0.0, _viewHeight * _viewHeight - _atmoBottom * _atmoBottom));
	
	float discriminant = _viewHeight * _viewHeight * (_viewZenithCosAngle * _viewZenithCosAngle - 1.0) +
		_atmoTop * _atmoTop;
	float d = max(0.0, (-_viewHeight * _viewZenithCosAngle + sqrt(discriminant))); // Distance to atmosphere boundary
	
	float d_min = _atmoTop - _viewHeight;
	float d_max = rho + H;
	float x_mu = (d - d_min) / (d_max - d_min);
	float x_r = rho / H;
	
	_uv = vec2(x_mu, x_r);
	
}

void uvToSkyViewLutParams(out float _viewZenithCosAngle, out float _lightViewCosAngle,
	uvec2 _viewSize, float _viewHeight, vec2 _uv, float _atmoBottom) {
	
	// Constrain uvs to valid sub texel range (avoid zenith derivative issue making LUT usage visible)
	_uv = vec2(fromSubUvsToUnit(_uv.x, _viewSize.x), fromSubUvsToUnit(_uv.y, _viewSize.y));
	
	float vHorizon = sqrt(_viewHeight * _viewHeight - _atmoBottom * _atmoBottom);
	float cosBeta = vHorizon / _viewHeight; // GroundToHorizonCos
	float beta = acos(cosBeta);
	float zenithHorizonAngle = PI - beta;
	
	if (_uv.y < 0.5) {
		
		float coord = 2.0 * _uv.y;
#if NONLINEARSKYVIEWLUT
		coord = 1.0 - coord;
		coord *= coord;
		coord = 1.0 - coord;
#endif //NONLINEARSKYVIEWLUT
		_viewZenithCosAngle = cos(zenithHorizonAngle * coord);
		
	} else {
		
		float coord = _uv.y * 2.0 - 1.0;
#if NONLINEARSKYVIEWLUT
		coord *= coord;
#endif //NONLINEARSKYVIEWLUT
		_viewZenithCosAngle = cos(zenithHorizonAngle + beta * coord);
		
	}
	
	float coord = _uv.x;
	coord *= coord;
	_lightViewCosAngle = -(coord * 2.0 - 1.0);
	
}

void skyViewLutParamsToUv(bool _intersectGround, float _viewZenithCosAngle,
	float _lightViewCosAngle, uvec2 _viewSize, float _viewHeight, out vec2 _uv, float _atmoBottom) {
		
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
	_uv = vec2(fromUnitToSubUvs(_uv.x, _viewSize.x), fromUnitToSubUvs(_uv.y, _viewSize.y));
	
}

float aerialPerspectiveDepthToSlice(float _depth) {
	
	return _depth * (1.0 / AP_KM_PER_SLICE);
	
}

float aerialPerspectiveSliceToDepth(float _slice) {
	
	return _slice * AP_KM_PER_SLICE;
	
}

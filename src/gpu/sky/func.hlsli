#ifndef SKY_FUNC_HLSLI
#define SKY_FUNC_HLSLI

#include "sky/access.hlsli"
#include "sky/types.hlsli"
#include "constants.hlsli"

struct MediumSampleRGB {
	float3 scattering;
	float3 absorption;
	float3 extinction;
	
	float3 scatteringMie;
	float3 absorptionMie;
	float3 extinctionMie;
	
	float3 scatteringRay;
	float3 absorptionRay;
	float3 extinctionRay;
	
	float3 scatteringOzo;
	float3 absorptionOzo;
	float3 extinctionOzo;
	
	float3 albedo;
};

struct SingleScatteringResult {
	float3 L; // Scattered light (luminance)
	float3 opticalDepth; // Optical depth (1/m)
	float3 transmittance; // Transmittance in [0,1] (unitless)
	float3 multiScatAs1;
	
	float3 newMultiScatStep0Out;
	float3 newMultiScatStep1Out;
};

static const float PlanetRadiusOffset = 0.01;
static const float RaymarchMinSPP = 4.0;
static const float RaymarchMaxSPP = 14.0;
static const float SunRadius = 0.5 * 0.505 * (PI / 180.0);

// - r0: ray origin
// - rd: normalized ray direction
// - s0: sphere center
// - sR: sphere radius
// - Returns distance from r0 to first intersecion with sphere,
//   or -1.0 if no intersection.
float raySphereIntersectNearest(float3 _r0, float3 _rd, float3 _s0, float _sR) {
	
	float a = dot(_rd, _rd);
	float3 s0_r0 = _r0 - _s0;
	float b = 2.0 * dot(_rd, s0_r0);
	float c = dot(s0_r0, s0_r0) - (_sR * _sR);
	float delta = b * b - 4.0*a*c;
	if (delta < 0.0 || a == 0.0)
		return -1.0;
	
	float sol0 = (-b - sqrt(delta)) / (2.0*a);
	float sol1 = (-b + sqrt(delta)) / (2.0*a);
	if (sol0 < 0.0 && sol1 < 0.0)
		return -1.0;
	if (sol0 < 0.0)
		return max(0.0, sol1);
	else if (sol1 < 0.0)
		return max(0.0, sol0);
	
	return max(0.0, min(sol0, sol1));
	
}

float cornetteShanksMiePhaseFunction(float _g, float _cosTheta) {
	
	float k = 3.0 / (8.0 * PI) * (1.0 - _g * _g) / (2.0 + _g * _g);
	return k * (1.0 + _cosTheta * _cosTheta) / pow(1.0 + _g * _g - 2.0 * _g * -_cosTheta, 1.5);
	
}

float rayleighPhase(float _cosTheta) {
	
	float factor = 3.0 / (16.0 * PI);
	return factor * (1.0 + _cosTheta * _cosTheta);
	
}

float3 getAlbedo(float3 _scattering, float3 _extinction) {
	
	return _scattering / max(float3(0.001, 0.001, 0.001), _extinction);
	
}

bool moveToTopAtmosphere(inout float3 _worldPos, float3 _worldDir, float _atmosphereTopRadius) {
	
	float viewHeight = length(_worldPos);
	if (viewHeight > _atmosphereTopRadius) {
		float tTop = raySphereIntersectNearest(_worldPos, _worldDir, float3(0.0, 0.0, 0.0), _atmosphereTopRadius);
		if (tTop >= 0.0) {
			float3 upVector = _worldPos / viewHeight;
			float3 upOffset = upVector * -PlanetRadiusOffset;
			_worldPos = _worldPos + _worldDir * tTop + upOffset;
		} else {
			// Ray is not intersecting the atmosphere
			return false;
		}
	}
	
	return true; // ok to start tracing
	
}

MediumSampleRGB sampleMediumRGB(AtmosphereParams _params, float3 _worldPos) {
	
	float viewHeight = length(_worldPos) - _params.bottomRadius;
	
	float densityMie = exp(_params.mieDensityExpScale * viewHeight);
	float densityRay = exp(_params.rayleighDensityExpScale * viewHeight);
	float densityOzo = saturate(viewHeight < _params.absorptionDensity0LayerWidth?
		_params.absorptionDensity0LinearTerm * viewHeight + _params.absorptionDensity0ConstantTerm :
		_params.absorptionDensity1LinearTerm * viewHeight + _params.absorptionDensity1ConstantTerm);
	
	MediumSampleRGB s;
	
	s.scatteringMie = densityMie * _params.mieScattering;
	s.absorptionMie = densityMie * _params.mieAbsorption;
	s.extinctionMie = densityMie * _params.mieExtinction;
	
	s.scatteringRay = densityRay * _params.rayleighScattering;
	s.absorptionRay = float3(0.0, 0.0, 0.0);
	s.extinctionRay = s.scatteringRay + s.absorptionRay;
	
	s.scatteringOzo = float3(0.0, 0.0, 0.0);
	s.absorptionOzo = densityOzo * _params.absorptionExtinction;
	s.extinctionOzo = s.scatteringOzo + s.absorptionOzo;
	
	s.scattering = s.scatteringMie + s.scatteringRay + s.scatteringOzo;
	s.absorption = s.absorptionMie + s.absorptionRay + s.absorptionOzo;
	s.extinction = s.extinctionMie + s.extinctionRay + s.extinctionOzo;
	s.albedo = getAlbedo(s.scattering, s.extinction);
	
	return s;
	
}

#ifdef TRANSMITTANCE_TEX
float3 getSunLuminance(AtmosphereParams _params, float3 _worldPos, float3 _worldDir,
	float3 _sunDirection, float3 _sunIlluminance) {
	
	if (dot(_worldDir, _sunDirection) <= cos(SunRadius))
		return float3(0.0, 0.0, 0.0);
	
	float t = raySphereIntersectNearest(_worldPos, _worldDir, float3(0.0, 0.0, 0.0), _params.bottomRadius);
	if (t >= 0.0)
		return float3(0.0, 0.0, 0.0); // no intersection
	
	float2 uvUp;
	lutTransmittanceParamsToUv(uvUp, _params, _params.bottomRadius, 1.0);
	
	float pHeight = length(_worldPos);
	float3 upVector = _worldPos / pHeight;
	float sunZenithCosAngle = dot(_sunDirection, upVector);
	float2 uvSun;
	lutTransmittanceParamsToUv(uvSun, _params, pHeight, sunZenithCosAngle);
	
	float cosAngle = dot(_worldDir, _sunDirection);
	float angle = acos(clamp(cosAngle, -1.0, 1.0));
	float radiusRatio = angle / SunRadius;
	float limbDarkening = sqrt(clamp(1.0 - radiusRatio*radiusRatio, 0.0001, 1.0));
	
	float3 sunLuminanceInSpace = _sunIlluminance / TRANSMITTANCE_TEX.SampleLevel(TRANSMITTANCE_SMP, uvUp, 0.0).rgb;
	return sunLuminanceInSpace * TRANSMITTANCE_TEX.SampleLevel(TRANSMITTANCE_SMP, uvSun, 0.0).rgb * limbDarkening;
	
}
#endif

#ifdef MULTI_SCATTERING_TEX
float3 getMultipleScattering(AtmosphereParams _params, float3 _worldPos, float _viewZenithCosAngle) {
	
	uint width;
	uint height;
	MULTI_SCATTERING_TEX.GetDimensions(width, height);
	uint2 size = {width, height};
	
	float2 uv = saturate(float2(
			_viewZenithCosAngle * 0.5 + 0.5,
			(length(_worldPos) - _params.bottomRadius) / (_params.topRadius - _params.bottomRadius)));
	uv = float2(fromUnitToSubUvs(uv.x, size.x), fromUnitToSubUvs(uv.y, size.y));
	
	return MULTI_SCATTERING_TEX.SampleLevel(MULTI_SCATTERING_SMP, uv, 0).rgb;
	
}
#endif

SingleScatteringResult integrateScatteredLuminance(AtmosphereParams _params,
	float3 _worldPos, float3 _worldDir, float3 _sunDir, bool _ground,
	float _sampleCountIni, bool _variableSampleCount, bool _mieRayPhase,
	float _tMaxMax, float3 _sunIlluminance) {
		
	SingleScatteringResult result = (SingleScatteringResult)0;
	
	// Compute next intersection with atmosphere or ground
	float3 earthO = {0.0, 0.0, 0.0};
	float tBottom = raySphereIntersectNearest(_worldPos, _worldDir, earthO, _params.bottomRadius);
	float tTop = raySphereIntersectNearest(_worldPos, _worldDir, earthO, _params.topRadius);
	float tMax = 0.0;
	if (tBottom < 0.0) {
		if (tTop < 0.0) {
			tMax = 0.0; // No intersection with earth nor atmosphere: stop right away
			return result;
		} else {
			tMax = tTop;
		}
	} else if (tTop > 0.0) {
		tMax = min(tTop, tBottom);
	}
	tMax = min(tMax, _tMaxMax);
	
	// Sample count
	float sampleCount = _sampleCountIni;
	float sampleCountFloor = _sampleCountIni;
	float tMaxFloor = tMax;
	if (_variableSampleCount) {
		sampleCount = lerp(RaymarchMinSPP, RaymarchMaxSPP, saturate(tMax * 0.01));
		sampleCountFloor = floor(sampleCount);
		tMaxFloor = tMax * sampleCountFloor / sampleCount; // rescale tMax to map to the last entire step segment.
	}
	float dt = tMax / sampleCount;
	
	// Phase functions
	float uniformPhase = rcp(4.0 * PI);
	float3 wi = _sunDir;
	float3 wo = _worldDir;
	float cosTheta = dot(wi, wo);
	float miePhaseValue = cornetteShanksMiePhaseFunction(_params.miePhaseG, -cosTheta); // negate cosTheta due to WorldDir being a "in" direction.
	float rayleighPhaseValue = rayleighPhase(cosTheta);
	
	// When building the scattering factor, we assume light illuminance is 1 to compute a transfer function relative to identity illuminance of 1.
	// This make the scattering factor independent of the light. It is now only linked to the atmosphere properties.
	float3 globalL = _sunIlluminance;
	
	// Ray march the atmosphere to integrate optical depth
	float3 L = {0.0, 0.0, 0.0};
	float3 throughput = {1.0, 1.0, 1.0};
	float3 opticalDepth = {0.0, 0.0, 0.0};
	float t = 0.0;
	float tPrev = 0.0;
	float sampleSegmentT = 0.3;
	for (float s = 0.0; s < sampleCount; s += 1.0) {
		
		if (_variableSampleCount) {
			// More expensive but artifact-free
			float t0 = s / sampleCountFloor;
			float t1 = (s + 1.0) / sampleCountFloor;
			// Non linear distribution of sample within the range.
			t0 = t0 * t0;
			t1 = t1 * t1;
			// Make t0 and t1 world space distances.
			t0 = tMaxFloor * t0;
			if (t1 > 1.0) {
				t1 = tMax;
				//	t1 = tMaxFloor;	// this reveals depth slices
			} else {
				t1 = tMaxFloor * t1;
			}
			t = t0 + (t1 - t0) * sampleSegmentT;
			dt = t1 - t0;
		} else {
			// Exact difference, important for accuracy of multiple scattering
			float newT = tMax * (s + sampleSegmentT) / sampleCount;
			dt = newT - t;
			t = newT;
		}
		float3 P = _worldPos + t * _worldDir;
		
		MediumSampleRGB medium = sampleMediumRGB(_params, P);
		float3 sampleOpticalDepth = medium.extinction * dt;
		float3 sampleTransmittance = exp(-sampleOpticalDepth);
		opticalDepth += sampleOpticalDepth;
		
		float pHeight = length(P);
		float3 upVector = P / pHeight;
		float sunZenithCosAngle = dot(_sunDir, upVector);
		float2 uv;
		lutTransmittanceParamsToUv(uv, _params, pHeight, sunZenithCosAngle);
#ifdef TRANSMITTANCE_TEX
		float3 transmittanceToSun = TRANSMITTANCE_TEX.SampleLevel(TRANSMITTANCE_SMP, uv, 0).rgb;
#else
		float3 transmittanceToSun = {0.0, 0.0, 0.0};
#endif
		
		float3 phaseTimesScattering;
		if (_mieRayPhase)
			phaseTimesScattering = medium.scatteringMie * miePhaseValue +
				medium.scatteringRay * rayleighPhaseValue;
		else
			phaseTimesScattering = medium.scattering * uniformPhase;
		
		// Earth shadow
		float tEarth = raySphereIntersectNearest(P, _sunDir,
			earthO + PlanetRadiusOffset * upVector, _params.bottomRadius);
		float earthShadow = tEarth >= 0.0 ? 0.0 : 1.0;
		
		// Dual scattering for multi scattering
		
		float3 multiScatteredLuminance = {0.0, 0.0, 0.0};
#ifdef MULTI_SCATTERING_TEX
		multiScatteredLuminance = getMultipleScattering(_params, P, sunZenithCosAngle);
#endif
		
		float3 S = globalL * (earthShadow * transmittanceToSun * phaseTimesScattering +
			multiScatteredLuminance * medium.scattering);
		
		// When using the power serie to accumulate all scattering order, serie r must be <1 for a serie to converge.
		// Under extreme coefficient, MultiScatAs1 can grow larger and thus result in broken visuals.
		// The way to fix that is to use a proper analytical integration as proposed in slide 28 of http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/
		// However, it is possible to disable as it can also work using simple power serie sum unroll up to 5th order. The rest of the orders has a really low contribution.
		
		float3 MS = medium.scattering * 1;
		float3 MSint = (MS - MS * sampleTransmittance) / medium.extinction;
		result.multiScatAs1 += throughput * MSint;
		
		// Evaluate input to multi scattering
		float3 newMS;
		
		newMS = earthShadow * transmittanceToSun * medium.scattering * uniformPhase * 1;
		result.newMultiScatStep0Out += throughput * (newMS - newMS * sampleTransmittance) / medium.extinction;
		
		newMS = medium.scattering * uniformPhase * multiScatteredLuminance;
		result.newMultiScatStep1Out += throughput * (newMS - newMS * sampleTransmittance) / medium.extinction;
		
		// See slide 28 at http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/
		float3 Sint = (S - S * sampleTransmittance) / medium.extinction; // integrate along the current step segment
		L += throughput * Sint; // accumulate and also take into account the transmittance from previous steps
		throughput *= sampleTransmittance;
		
		tPrev = t;
		
	}
	
	if (_ground && tMax == tBottom && tBottom > 0.0) {
		
		// Account for bounced light off the earth
		float3 P = _worldPos + tBottom * _worldDir;
		float pHeight = length(P);
		
		float3 upVector = P / pHeight;
		float sunZenithCosAngle = dot(_sunDir, upVector);
		float2 uv;
		lutTransmittanceParamsToUv(uv, _params, pHeight, sunZenithCosAngle);
#ifdef TRANSMITTANCE_TEX
		float3 transmittanceToSun = TRANSMITTANCE_TEX.SampleLevel(TRANSMITTANCE_SMP,  uv, 0).rgb;
#else
		float3 transmittanceToSun = {0.0, 0.0, 0.0};
#endif
		
		float NdotL = saturate(dot(normalize(upVector), normalize(_sunDir)));
		L += globalL * transmittanceToSun * throughput * NdotL * _params.groundAlbedo / PI;
		
	}
	
	result.L = L;
	result.opticalDepth = opticalDepth;
	result.transmittance = throughput;
	return result;
	
}

#endif

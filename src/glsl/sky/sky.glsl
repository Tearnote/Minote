// Common functions for all atmospheric shaders

#include "../constants.glsl"
#include "skyAccess.glsl"

#ifndef MULTIPLE_SCATTERING_ENABLED
#define MULTIPLE_SCATTERING_ENABLED 0
#endif

#define PLANET_RADIUS_OFFSET 0.01

#define RAYMARCH_MIN_SPP 4.0
#define RAYMARCH_MAX_SPP 14.0

struct MediumSampleRGB {
	
	vec3 scattering;
	vec3 absorption;
	vec3 extinction;
	
	vec3 scatteringMie;
	vec3 absorptionMie;
	vec3 extinctionMie;
	
	vec3 scatteringRay;
	vec3 absorptionRay;
	vec3 extinctionRay;
	
	vec3 scatteringOzo;
	vec3 absorptionOzo;
	vec3 extinctionOzo;
	
	vec3 albedo;
	
};

struct SingleScatteringResult {
	
	vec3 L; // Scattered light (luminance)
	vec3 opticalDepth; // Optical depth (1/m)
	vec3 transmittance; // Transmittance in [0,1] (unitless)
	vec3 multiScatAs1;
	
	vec3 newMultiScatStep0Out;
	vec3 newMultiScatStep1Out;
	
};

struct AtmosphereParams {
	
	float bottomRadius; // Radius of the planet (center to ground)
	float topRadius; // Maximum considered atmosphere height (center to atmosphere top)
	
	float rayleighDensityExpScale; // Rayleigh scattering exponential distribution scale in the atmosphere
	float pad0;
	vec3 rayleighScattering; // Rayleigh scattering coefficients
	
	float mieDensityExpScale; // Mie scattering exponential distribution scale in the atmosphere
	vec3 mieScattering; // Mie scattering coefficients
	float pad1;
	vec3 mieExtinction; // Mie extinction coefficients
	float pad2;
	vec3 mieAbsorption; // Mie absorption coefficients
	float miePhaseG; // Mie phase function excentricity
	
	// Another medium type in the atmosphere
	float absorptionDensity0LayerWidth;
	float absorptionDensity0ConstantTerm;
	float absorptionDensity0LinearTerm;
	float absorptionDensity1ConstantTerm;
	float absorptionDensity1LinearTerm;
	float pad3;
	float pad4;
	float pad5;
	vec3 absorptionExtinction; // This other medium only absorb light, e.g. useful to represent ozone in the earth atmosphere
	float pad6;
	
	vec3 groundAlbedo; // The albedo of the ground.
	
};

layout(set = 0, binding = 1) uniform Atmosphere {
	AtmosphereParams u_atmo;
};

#ifndef TRANSMITTANCE_DISABLED
layout(set = 0, binding = 2) uniform sampler2D s_transmittanceLut;
#endif //TRANSMITTANCE_DISABLED
#if MULTIPLE_SCATTERING_ENABLED
layout(set = 0, binding = 3) uniform sampler2D s_multiScatteringLut;
#endif //MULTIPLE_SCATTERING_ENABLED

vec3 getAlbedo(vec3 _scattering, vec3 _extinction) {
	
	return _scattering / max(vec3(0.001), _extinction);
	
}

float cornetteShanksMiePhaseFunction(float _g, float _cosTheta) {
	
	float k = 3.0 / (8.0 * PI) * (1.0 - _g * _g) / (2.0 + _g * _g);
	return k * (1.0 + _cosTheta * _cosTheta) / pow(1.0 + _g * _g - 2.0 * _g * -_cosTheta, 1.5);
	
}

float rayleighPhase(float _cosTheta) {
	
	float factor = 3.0 / (16.0 * PI);
	return factor * (1.0 + _cosTheta * _cosTheta);
	
}

// - r0: ray origin
// - rd: normalized ray direction
// - s0: sphere center
// - sR: sphere radius
// - Returns distance from r0 to first intersecion with sphere,
//   or -1.0 if no intersection.
float raySphereIntersectNearest(vec3 _r0, vec3 _rd, vec3 _s0, float _sR) {
	
	float a = dot(_rd, _rd);
	vec3 s0_r0 = _r0 - _s0;
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

bool moveToTopAtmosphere(inout vec3 _worldPos, vec3 _worldDir, float _atmosphereTopRadius) {
	
	float viewHeight = length(_worldPos);
	if (viewHeight > _atmosphereTopRadius) {
		
		float tTop = raySphereIntersectNearest(_worldPos, _worldDir, vec3(0.0), _atmosphereTopRadius);
		if (tTop >= 0.0) {
			
			vec3 upVector = _worldPos / viewHeight;
			vec3 upOffset = upVector * -PLANET_RADIUS_OFFSET;
			_worldPos = _worldPos + _worldDir * tTop + upOffset;
			
		} else {
			
			// Ray is not intersecting the atmosphere
			return false;
			
		}
		
	}
	
	return true; // ok to start tracing
	
}

MediumSampleRGB sampleMediumRGB(vec3 _worldPos) {
	
	const float viewHeight = length(_worldPos) - u_atmo.bottomRadius;
	
	const float densityMie = exp(u_atmo.mieDensityExpScale * viewHeight);
	const float densityRay = exp(u_atmo.rayleighDensityExpScale * viewHeight);
	const float densityOzo = clamp(viewHeight < u_atmo.absorptionDensity0LayerWidth ?
		u_atmo.absorptionDensity0LinearTerm * viewHeight + u_atmo.absorptionDensity0ConstantTerm :
		u_atmo.absorptionDensity1LinearTerm * viewHeight + u_atmo.absorptionDensity1ConstantTerm, 0.0, 1.0);
	
	MediumSampleRGB s;
	
	s.scatteringMie = densityMie * u_atmo.mieScattering;
	s.absorptionMie = densityMie * u_atmo.mieAbsorption;
	s.extinctionMie = densityMie * u_atmo.mieExtinction;
	
	s.scatteringRay = densityRay * u_atmo.rayleighScattering;
	s.absorptionRay = vec3(0.0);
	s.extinctionRay = s.scatteringRay + s.absorptionRay;
	
	s.scatteringOzo = vec3(0.0);
	s.absorptionOzo = densityOzo * u_atmo.absorptionExtinction;
	s.extinctionOzo = s.scatteringOzo + s.absorptionOzo;
	
	s.scattering = s.scatteringMie + s.scatteringRay + s.scatteringOzo;
	s.absorption = s.absorptionMie + s.absorptionRay + s.absorptionOzo;
	s.extinction = s.extinctionMie + s.extinctionRay + s.extinctionOzo;
	s.albedo = getAlbedo(s.scattering, s.extinction);
	
	return s;
	
}

#ifndef TRANSMITTANCE_DISABLED

vec3 getSunLuminance(vec3 _worldPos, vec3 _worldDir, vec3 _sunDirection, vec3 _sunIlluminance) {
	
	const float SunRadius = 0.5 * 0.505 * 3.14159 / 180.0;
	
	if (dot(_worldDir, _sunDirection) > cos(SunRadius)) {
		
		float t = raySphereIntersectNearest(_worldPos, _worldDir, vec3(0.0), u_atmo.bottomRadius);
		if (t < 0.0) { // no intersection
			
			vec2 uvUp;
			lutTransmittanceParamsToUv(u_atmo.bottomRadius, 1.0, uvUp, u_atmo.bottomRadius, u_atmo.topRadius);
			
			float pHeight = length(_worldPos);
			const vec3 upVector = _worldPos / pHeight;
			float sunZenithCosAngle = dot(_sunDirection, upVector);
			vec2 uvSun;
			lutTransmittanceParamsToUv(pHeight, sunZenithCosAngle, uvSun, u_atmo.bottomRadius, u_atmo.topRadius);
			
			float cosAngle = dot(_worldDir, _sunDirection);
			float angle = acos(clamp(cosAngle, -1.0, 1.0));
			float radiusRatio = angle / (SunRadius);
			float limbDarkening = sqrt(clamp(1.0 - radiusRatio*radiusRatio, 0.0001, 1.0));
			
			vec3 sunLuminanceInSpace = _sunIlluminance / textureLod(s_transmittanceLut, uvUp, 0.0).rgb;
			return sunLuminanceInSpace * textureLod(s_transmittanceLut, uvSun, 0.0).rgb * limbDarkening;
			
		}
		
	}
	
	return vec3(0.0);
	
}

#endif //TRANSMITTANCE_DISABLED

#if MULTIPLE_SCATTERING_ENABLED

vec3 getMultipleScattering(vec3 _worldPos, float _viewZenithCosAngle) {
	
	uvec2 size = textureSize(s_multiScatteringLut, 0).xy;
	vec2 uv = clamp(vec2(
			_viewZenithCosAngle * 0.5 + 0.5,
			(length(_worldPos) - u_atmo.bottomRadius) / (u_atmo.topRadius - u_atmo.bottomRadius)),
		0.0, 1.0);
	uv = vec2(fromUnitToSubUvs(uv.x, size.x), fromUnitToSubUvs(uv.y, size.y));
	
	vec3 multiScatteredLuminance = textureLod(s_multiScatteringLut, uv, 0.0).rgb;
	return multiScatteredLuminance;
	
}

#endif //MULTIPLE_SCATTERING_ENABLED

SingleScatteringResult integrateScatteredLuminance(vec3 _worldPos, vec3 _worldDir, vec3 _sunDir,
	bool _ground, float _sampleCountIni, bool _variableSampleCount, bool _mieRayPhase, float _tMaxMax,
	vec3 _sunIlluminance) {
		
	SingleScatteringResult result = {vec3(0), vec3(0), vec3(0), vec3(0), vec3(0), vec3(0)};
	
	// Compute next intersection with atmosphere or ground
	vec3 earthO = vec3(0.0);
	float tBottom = raySphereIntersectNearest(_worldPos, _worldDir, earthO, u_atmo.bottomRadius);
	float tTop = raySphereIntersectNearest(_worldPos, _worldDir, earthO, u_atmo.topRadius);
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
		
		sampleCount = mix(RAYMARCH_MIN_SPP, RAYMARCH_MAX_SPP, clamp(tMax * 0.01, 0.0, 1.0));
		sampleCountFloor = floor(sampleCount);
		tMaxFloor = tMax * sampleCountFloor / sampleCount; // rescale tMax to map to the last entire step segment.
		
	}
	float dt = tMax / sampleCount;
	
	// Phase functions
	const float uniformPhase = 1.0 / (4.0 * PI);
	const vec3 wi = _sunDir;
	const vec3 wo = _worldDir;
	float cosTheta = dot(wi, wo);
	float miePhaseValue = cornetteShanksMiePhaseFunction(u_atmo.miePhaseG, -cosTheta); // negate cosTheta due to WorldDir being a "in" direction.
	float rayleighPhaseValue = rayleighPhase(cosTheta);
	
	// When building the scattering factor, we assume light illuminance is 1 to compute a transfer function relative to identity illuminance of 1.
	// This make the scattering factor independent of the light. It is now only linked to the atmosphere properties.
	vec3 globalL = _sunIlluminance;
	
	// Ray march the atmosphere to integrate optical depth
	vec3 L = vec3(0.0);
	vec3 throughput = vec3(1.0);
	vec3 opticalDepth = vec3(0.0);
	float t = 0.0;
	float tPrev = 0.0;
	const float sampleSegmentT = 0.3;
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
		vec3 P = _worldPos + t * _worldDir;
		
		MediumSampleRGB medium = sampleMediumRGB(P);
		const vec3 sampleOpticalDepth = medium.extinction * dt;
		const vec3 sampleTransmittance = exp(-sampleOpticalDepth);
		opticalDepth += sampleOpticalDepth;
		
		float pHeight = length(P);
		const vec3 upVector = P / pHeight;
		float sunZenithCosAngle = dot(_sunDir, upVector);
		vec2 uv;
		lutTransmittanceParamsToUv(pHeight, sunZenithCosAngle, uv, u_atmo.bottomRadius, u_atmo.topRadius);
#ifndef TRANSMITTANCE_DISABLED
		vec3 transmittanceToSun = textureLod(s_transmittanceLut, uv, 0.0).rgb;
#else //TRANSMITTANCE_DISABLED
		vec3 transmittanceToSun = vec3(0.0);
#endif //TRANSMITTANCE_DISABLED
		
		vec3 phaseTimesScattering;
		if (_mieRayPhase)
			phaseTimesScattering = medium.scatteringMie * miePhaseValue +
				medium.scatteringRay * rayleighPhaseValue;
		else
			phaseTimesScattering = medium.scattering * uniformPhase;
		
		// Earth shadow
		float tEarth = raySphereIntersectNearest(P, _sunDir,
			earthO + PLANET_RADIUS_OFFSET * upVector, u_atmo.bottomRadius);
		float earthShadow = tEarth >= 0.0 ? 0.0 : 1.0;
		
		// Dual scattering for multi scattering
		
		vec3 multiScatteredLuminance = vec3(0.0);
#if MULTIPLE_SCATTERING_ENABLED
		multiScatteredLuminance = getMultipleScattering(P, sunZenithCosAngle);
#endif
		
		vec3 S = globalL * (earthShadow * transmittanceToSun * phaseTimesScattering +
			multiScatteredLuminance * medium.scattering);
		
		// When using the power serie to accumulate all scattering order, serie r must be <1 for a serie to converge.
		// Under extreme coefficient, MultiScatAs1 can grow larger and thus result in broken visuals.
		// The way to fix that is to use a proper analytical integration as proposed in slide 28 of http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/
		// However, it is possible to disable as it can also work using simple power serie sum unroll up to 5th order. The rest of the orders has a really low contribution.
		
		vec3 MS = medium.scattering * 1;
		vec3 MSint = (MS - MS * sampleTransmittance) / medium.extinction;
		result.multiScatAs1 += throughput * MSint;
		
		// Evaluate input to multi scattering
		vec3 newMS;
		
		newMS = earthShadow * transmittanceToSun * medium.scattering * uniformPhase * 1;
		result.newMultiScatStep0Out += throughput * (newMS - newMS * sampleTransmittance) / medium.extinction;
		
		newMS = medium.scattering * uniformPhase * multiScatteredLuminance;
		result.newMultiScatStep1Out += throughput * (newMS - newMS * sampleTransmittance) / medium.extinction;
		
		// See slide 28 at http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/
		vec3 Sint = (S - S * sampleTransmittance) / medium.extinction; // integrate along the current step segment
		L += throughput * Sint; // accumulate and also take into account the transmittance from previous steps
		throughput *= sampleTransmittance;
		
		tPrev = t;
		
	}
	
	if (_ground && tMax == tBottom && tBottom > 0.0) {
		
		// Account for bounced light off the earth
		vec3 P = _worldPos + tBottom * _worldDir;
		float pHeight = length(P);
		
		const vec3 upVector = P / pHeight;
		float sunZenithCosAngle = dot(_sunDir, upVector);
		vec2 uv;
		lutTransmittanceParamsToUv(pHeight, sunZenithCosAngle, uv, u_atmo.bottomRadius, u_atmo.topRadius);
#ifndef TRANSMITTANCE_DISABLED
		vec3 transmittanceToSun = textureLod(s_transmittanceLut, uv, 0.0).rgb;
#else //TRANSMITTANCE_DISABLED
		vec3 transmittanceToSun = vec3(0.0);
#endif //TRANSMITTANCE_DISABLED
		
		const float NdotL = clamp(dot(normalize(upVector), normalize(_sunDir)), 0.0, 1.0);
		L += globalL * transmittanceToSun * throughput * NdotL * u_atmo.groundAlbedo / PI;
		
	}
	
	result.L = L;
	result.opticalDepth = opticalDepth;
	result.transmittance = throughput;
	return result;
	
}

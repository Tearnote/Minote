// Common functions for all atmospheric shaders

#ifndef SKY_GLSL
#define SKY_GLSL

#include "../constants.glsl"
#include "skyAccess.glsl"
#include "skyTypes.glsl"

#ifndef U_ATMO
#error "Atmosphere params uniform name needs to be defined"
#endif //U_ATMO

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
	
	float viewHeight = length(_worldPos) - U_ATMO.bottomRadius;
	
	float densityMie = exp(U_ATMO.mieDensityExpScale * viewHeight);
	float densityRay = exp(U_ATMO.rayleighDensityExpScale * viewHeight);
	float densityOzo = clamp(viewHeight < U_ATMO.absorptionDensity0LayerWidth ?
		U_ATMO.absorptionDensity0LinearTerm * viewHeight + U_ATMO.absorptionDensity0ConstantTerm :
		U_ATMO.absorptionDensity1LinearTerm * viewHeight + U_ATMO.absorptionDensity1ConstantTerm, 0.0, 1.0);
	
	MediumSampleRGB s;
	
	s.scatteringMie = densityMie * U_ATMO.mieScattering;
	s.absorptionMie = densityMie * U_ATMO.mieAbsorption;
	s.extinctionMie = densityMie * U_ATMO.mieExtinction;
	
	s.scatteringRay = densityRay * U_ATMO.rayleighScattering;
	s.absorptionRay = vec3(0.0);
	s.extinctionRay = s.scatteringRay + s.absorptionRay;
	
	s.scatteringOzo = vec3(0.0);
	s.absorptionOzo = densityOzo * U_ATMO.absorptionExtinction;
	s.extinctionOzo = s.scatteringOzo + s.absorptionOzo;
	
	s.scattering = s.scatteringMie + s.scatteringRay + s.scatteringOzo;
	s.absorption = s.absorptionMie + s.absorptionRay + s.absorptionOzo;
	s.extinction = s.extinctionMie + s.extinctionRay + s.extinctionOzo;
	s.albedo = getAlbedo(s.scattering, s.extinction);
	
	return s;
	
}

#ifdef S_TRANSMITTANCE

vec3 getSunLuminance(vec3 _worldPos, vec3 _worldDir, vec3 _sunDirection, vec3 _sunIlluminance) {
	
	float SunRadius = 0.5 * 0.505 * 3.14159 / 180.0;
	
	if (dot(_worldDir, _sunDirection) > cos(SunRadius)) {
		
		float t = raySphereIntersectNearest(_worldPos, _worldDir, vec3(0.0), U_ATMO.bottomRadius);
		if (t < 0.0) { // no intersection
			
			vec2 uvUp;
			lutTransmittanceParamsToUv(U_ATMO.bottomRadius, 1.0, uvUp, U_ATMO.bottomRadius, U_ATMO.topRadius);
			
			float pHeight = length(_worldPos);
			vec3 upVector = _worldPos / pHeight;
			float sunZenithCosAngle = dot(_sunDirection, upVector);
			vec2 uvSun;
			lutTransmittanceParamsToUv(pHeight, sunZenithCosAngle, uvSun, U_ATMO.bottomRadius, U_ATMO.topRadius);
			
			float cosAngle = dot(_worldDir, _sunDirection);
			float angle = acos(clamp(cosAngle, -1.0, 1.0));
			float radiusRatio = angle / SunRadius;
			float limbDarkening = sqrt(clamp(1.0 - radiusRatio*radiusRatio, 0.0001, 1.0));
			
			vec3 sunLuminanceInSpace = _sunIlluminance / textureLod(S_TRANSMITTANCE, uvUp, 0.0).rgb;
			return sunLuminanceInSpace * textureLod(S_TRANSMITTANCE, uvSun, 0.0).rgb * limbDarkening;
			
		}
		
	}
	
	return vec3(0.0);
	
}

#endif //S_TRANSMITTANCE

#ifdef S_MULTISCATTERING

vec3 getMultipleScattering(vec3 _worldPos, float _viewZenithCosAngle) {
	
	uvec2 size = textureSize(S_MULTISCATTERING, 0).xy;
	vec2 uv = clamp(vec2(
			_viewZenithCosAngle * 0.5 + 0.5,
			(length(_worldPos) - U_ATMO.bottomRadius) / (U_ATMO.topRadius - U_ATMO.bottomRadius)),
		0.0, 1.0);
	uv = vec2(fromUnitToSubUvs(uv.x, size.x), fromUnitToSubUvs(uv.y, size.y));
	
	vec3 multiScatteredLuminance = textureLod(S_MULTISCATTERING, uv, 0.0).rgb;
	return multiScatteredLuminance;
	
}

#endif //S_MULTISCATTERING

SingleScatteringResult integrateScatteredLuminance(vec3 _worldPos, vec3 _worldDir, vec3 _sunDir,
	bool _ground, float _sampleCountIni, bool _variableSampleCount, bool _mieRayPhase, float _tMaxMax,
	vec3 _sunIlluminance) {
		
	SingleScatteringResult result = {vec3(0), vec3(0), vec3(0), vec3(0), vec3(0), vec3(0)};
	
	// Compute next intersection with atmosphere or ground
	vec3 earthO = vec3(0.0);
	float tBottom = raySphereIntersectNearest(_worldPos, _worldDir, earthO, U_ATMO.bottomRadius);
	float tTop = raySphereIntersectNearest(_worldPos, _worldDir, earthO, U_ATMO.topRadius);
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
	float uniformPhase = 1.0 / (4.0 * PI);
	vec3 wi = _sunDir;
	vec3 wo = _worldDir;
	float cosTheta = dot(wi, wo);
	float miePhaseValue = cornetteShanksMiePhaseFunction(U_ATMO.miePhaseG, -cosTheta); // negate cosTheta due to WorldDir being a "in" direction.
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
		vec3 P = _worldPos + t * _worldDir;
		
		MediumSampleRGB medium = sampleMediumRGB(P);
		vec3 sampleOpticalDepth = medium.extinction * dt;
		vec3 sampleTransmittance = exp(-sampleOpticalDepth);
		opticalDepth += sampleOpticalDepth;
		
		float pHeight = length(P);
		vec3 upVector = P / pHeight;
		float sunZenithCosAngle = dot(_sunDir, upVector);
		vec2 uv;
		lutTransmittanceParamsToUv(pHeight, sunZenithCosAngle, uv, U_ATMO.bottomRadius, U_ATMO.topRadius);
#ifdef S_TRANSMITTANCE
		vec3 transmittanceToSun = textureLod(S_TRANSMITTANCE, uv, 0.0).rgb;
#else //S_TRANSMITTANCE
		vec3 transmittanceToSun = vec3(0.0);
#endif //S_TRANSMITTANCE
		
		vec3 phaseTimesScattering;
		if (_mieRayPhase)
			phaseTimesScattering = medium.scatteringMie * miePhaseValue +
				medium.scatteringRay * rayleighPhaseValue;
		else
			phaseTimesScattering = medium.scattering * uniformPhase;
		
		// Earth shadow
		float tEarth = raySphereIntersectNearest(P, _sunDir,
			earthO + PLANET_RADIUS_OFFSET * upVector, U_ATMO.bottomRadius);
		float earthShadow = tEarth >= 0.0 ? 0.0 : 1.0;
		
		// Dual scattering for multi scattering
		
		vec3 multiScatteredLuminance = vec3(0.0);
#ifdef S_MULTISCATTERING
		multiScatteredLuminance = getMultipleScattering(P, sunZenithCosAngle);
#endif //S_MULTISCATTERING
		
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
		
		vec3 upVector = P / pHeight;
		float sunZenithCosAngle = dot(_sunDir, upVector);
		vec2 uv;
		lutTransmittanceParamsToUv(pHeight, sunZenithCosAngle, uv, U_ATMO.bottomRadius, U_ATMO.topRadius);
#ifdef S_TRANSMITTANCE
		vec3 transmittanceToSun = textureLod(S_TRANSMITTANCE, uv, 0.0).rgb;
#else //S_TRANSMITTANCE
		vec3 transmittanceToSun = vec3(0.0);
#endif //S_TRANSMITTANCE
		
		float NdotL = clamp(dot(normalize(upVector), normalize(_sunDir)), 0.0, 1.0);
		L += globalL * transmittanceToSun * throughput * NdotL * U_ATMO.groundAlbedo / PI;
		
	}
	
	result.L = L;
	result.opticalDepth = opticalDepth;
	result.transmittance = throughput;
	return result;
	
}

#endif //SKY_GLSL

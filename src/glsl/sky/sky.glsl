#define PI 3.1415926535897932384626433832795

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
	vec3 OpticalDepth; // Optical depth (1/m)
	vec3 Transmittance; // Transmittance in [0,1] (unitless)
	vec3 MultiScatAs1;

	vec3 NewMultiScatStep0Out;
	vec3 NewMultiScatStep1Out;

};

layout(set = 0, binding = 1) uniform AtmosphereParameters {

	float BottomRadius; // Radius of the planet (center to ground)
	float TopRadius; // Maximum considered atmosphere height (center to atmosphere top)

	float RayleighDensityExpScale; // Rayleigh scattering exponential distribution scale in the atmosphere
	float pad0;
	vec3 RayleighScattering; // Rayleigh scattering coefficients

	float MieDensityExpScale; // Mie scattering exponential distribution scale in the atmosphere
	vec3 MieScattering; // Mie scattering coefficients
	float pad1;
	vec3 MieExtinction; // Mie extinction coefficients
	float pad2;
	vec3 MieAbsorption; // Mie absorption coefficients
	float MiePhaseG; // Mie phase function excentricity

	// Another medium type in the atmosphere
	float AbsorptionDensity0LayerWidth;
	float AbsorptionDensity0ConstantTerm;
	float AbsorptionDensity0LinearTerm;
	float AbsorptionDensity1ConstantTerm;
	float AbsorptionDensity1LinearTerm;
	float pad3;
	float pad4;
	float pad5;
	vec3 AbsorptionExtinction; // This other medium only absorb light, e.g. useful to represent ozone in the earth atmosphere
	float pad6;

	vec3 GroundAlbedo; // The albedo of the ground.

} Atmosphere;

#ifndef TRANSMITTANCE_DISABLED
layout(set = 0, binding = 2) uniform sampler2D TransmittanceLutTexture;
#endif
#if MULTIPLE_SCATTERING_ENABLED
layout(set = 0, binding = 3) uniform sampler2D MultiScatteringLutTexture;
#endif

float fromUnitToSubUvs(float u, float resolution) {
	return (u + 0.5 / resolution) * (resolution / (resolution + 1.0));
}

float fromSubUvsToUnit(float u, float resolution) {
	return (u - 0.5 / resolution) * (resolution / (resolution - 1.0));
}

vec3 getAlbedo(vec3 scattering, vec3 extinction) {
	return scattering / max(vec3(0.001), extinction);
}

float CornetteShanksMiePhaseFunction(float g, float cosTheta) {
	float k = 3.0 / (8.0 * PI) * (1.0 - g * g) / (2.0 + g * g);
	return k * (1.0 + cosTheta * cosTheta) / pow(1.0 + g * g - 2.0 * g * -cosTheta, 1.5);
}

float RayleighPhase(float cosTheta) {
	float factor = 3.0 / (16.0 * PI);
	return factor * (1.0 + cosTheta * cosTheta);
}

// - r0: ray origin
// - rd: normalized ray direction
// - s0: sphere center
// - sR: sphere radius
// - Returns distance from r0 to first intersecion with sphere,
//   or -1.0 if no intersection.
float raySphereIntersectNearest(vec3 r0, vec3 rd, vec3 s0, float sR) {
	float a = dot(rd, rd);
	vec3 s0_r0 = r0 - s0;
	float b = 2.0 * dot(rd, s0_r0);
	float c = dot(s0_r0, s0_r0) - (sR * sR);
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

MediumSampleRGB sampleMediumRGB(in vec3 WorldPos) {
	const float viewHeight = length(WorldPos) - Atmosphere.BottomRadius;

	const float densityMie = exp(Atmosphere.MieDensityExpScale * viewHeight);
	const float densityRay = exp(Atmosphere.RayleighDensityExpScale * viewHeight);
	const float densityOzo = clamp(viewHeight < Atmosphere.AbsorptionDensity0LayerWidth ?
	Atmosphere.AbsorptionDensity0LinearTerm * viewHeight + Atmosphere.AbsorptionDensity0ConstantTerm :
	Atmosphere.AbsorptionDensity1LinearTerm * viewHeight + Atmosphere.AbsorptionDensity1ConstantTerm, 0.0, 1.0);

	MediumSampleRGB s;

	s.scatteringMie = densityMie * Atmosphere.MieScattering;
	s.absorptionMie = densityMie * Atmosphere.MieAbsorption;
	s.extinctionMie = densityMie * Atmosphere.MieExtinction;

	s.scatteringRay = densityRay * Atmosphere.RayleighScattering;
	s.absorptionRay = vec3(0.0);
	s.extinctionRay = s.scatteringRay + s.absorptionRay;

	s.scatteringOzo = vec3(0.0);
	s.absorptionOzo = densityOzo * Atmosphere.AbsorptionExtinction;
	s.extinctionOzo = s.scatteringOzo + s.absorptionOzo;

	s.scattering = s.scatteringMie + s.scatteringRay + s.scatteringOzo;
	s.absorption = s.absorptionMie + s.absorptionRay + s.absorptionOzo;
	s.extinction = s.extinctionMie + s.extinctionRay + s.extinctionOzo;
	s.albedo = getAlbedo(s.scattering, s.extinction);

	return s;
}

void UvToLutTransmittanceParams(out float viewHeight, out float viewZenithCosAngle, in vec2 uv) {
	float x_mu = uv.x;
	float x_r = uv.y;

	float H = sqrt(Atmosphere.TopRadius * Atmosphere.TopRadius - Atmosphere.BottomRadius * Atmosphere.BottomRadius);
	float rho = H * x_r;
	viewHeight = sqrt(rho * rho + Atmosphere.BottomRadius * Atmosphere.BottomRadius);

	float d_min = Atmosphere.TopRadius - viewHeight;
	float d_max = rho + H;
	float d = d_min + x_mu * (d_max - d_min);
	viewZenithCosAngle = d == 0.0 ? 1.0 : (H * H - rho * rho - d * d) / (2.0 * viewHeight * d);
	viewZenithCosAngle = clamp(viewZenithCosAngle, -1.0, 1.0);
}

void LutTransmittanceParamsToUv(in float viewHeight, in float viewZenithCosAngle, out vec2 uv) {
	float H = sqrt(max(0.0, Atmosphere.TopRadius * Atmosphere.TopRadius - Atmosphere.BottomRadius * Atmosphere.BottomRadius));
	float rho = sqrt(max(0.0, viewHeight * viewHeight - Atmosphere.BottomRadius * Atmosphere.BottomRadius));

	float discriminant = viewHeight * viewHeight * (viewZenithCosAngle * viewZenithCosAngle - 1.0) + Atmosphere.TopRadius * Atmosphere.TopRadius;
	float d = max(0.0, (-viewHeight * viewZenithCosAngle + sqrt(discriminant))); // Distance to atmosphere boundary

	float d_min = Atmosphere.TopRadius - viewHeight;
	float d_max = rho + H;
	float x_mu = (d - d_min) / (d_max - d_min);
	float x_r = rho / H;

	uv = vec2(x_mu, x_r);
}

#define NONLINEARSKYVIEWLUT 1
void UvToSkyViewLutParams(out float viewZenithCosAngle, out float lightViewCosAngle, in uvec2 viewSize, in float viewHeight, in vec2 uv) {
	// Constrain uvs to valid sub texel range (avoid zenith derivative issue making LUT usage visible)
	uv = vec2(fromSubUvsToUnit(uv.x, viewSize.x), fromSubUvsToUnit(uv.y, viewSize.y));

	float Vhorizon = sqrt(viewHeight * viewHeight - Atmosphere.BottomRadius * Atmosphere.BottomRadius);
	float CosBeta = Vhorizon / viewHeight; // GroundToHorizonCos
	float Beta = acos(CosBeta);
	float ZenithHorizonAngle = PI - Beta;

	if (uv.y < 0.5) {
		float coord = 2.0 * uv.y;
		coord = 1.0 - coord;
#if NONLINEARSKYVIEWLUT
		coord *= coord;
#endif
		coord = 1.0 - coord;
		viewZenithCosAngle = cos(ZenithHorizonAngle * coord);
	} else {
		float coord = uv.y * 2.0 - 1.0;
#if NONLINEARSKYVIEWLUT
		coord *= coord;
#endif
		viewZenithCosAngle = cos(ZenithHorizonAngle + Beta * coord);
	}

	float coord = uv.x;
	coord *= coord;
	lightViewCosAngle = -(coord*2.0 - 1.0);
}

void SkyViewLutParamsToUv(in bool IntersectGround, in float viewZenithCosAngle, in float lightViewCosAngle, in uvec2 viewSize, in float viewHeight, out vec2 uv) {
	float Vhorizon = sqrt(viewHeight * viewHeight - Atmosphere.BottomRadius * Atmosphere.BottomRadius);
	float CosBeta = Vhorizon / viewHeight; // GroundToHorizonCos
	float Beta = acos(CosBeta);
	float ZenithHorizonAngle = PI - Beta;

	if (!IntersectGround) {
		float coord = acos(viewZenithCosAngle) / ZenithHorizonAngle;
		coord = 1.0 - coord;
#if NONLINEARSKYVIEWLUT
		coord = sqrt(coord);
#endif
		coord = 1.0 - coord;
		uv.y = coord * 0.5;
	} else {
		float coord = (acos(viewZenithCosAngle) - ZenithHorizonAngle) / Beta;
#if NONLINEARSKYVIEWLUT
		coord = sqrt(coord);
#endif
		uv.y = coord * 0.5 + 0.5;
	}

	{
		float coord = -lightViewCosAngle * 0.5 + 0.5;
		coord = sqrt(coord);
		uv.x = coord;
	}

	// Constrain uvs to valid sub texel range (avoid zenith derivative issue making LUT usage visible)
	uv = vec2(fromUnitToSubUvs(uv.x, viewSize.x), fromUnitToSubUvs(uv.y, viewSize.y));
}

#ifndef TRANSMITTANCE_DISABLED
vec3 GetSunLuminance(vec3 WorldPos, vec3 WorldDir, float PlanetRadius,
	vec3 SunDirection, vec3 SunIlluminance) {
	if (dot(WorldDir, SunDirection) > cos(0.5*0.505*3.14159 / 180.0)) {
		float t = raySphereIntersectNearest(WorldPos, WorldDir, vec3(0.0), PlanetRadius);
		if (t < 0.0) { // no intersection
			vec2 uvUp;
			LutTransmittanceParamsToUv(Atmosphere.BottomRadius, 1.0, uvUp);
			
			float pHeight = length(WorldPos);
			const vec3 UpVector = WorldPos / pHeight;
			float SunZenithCosAngle = dot(SunDirection, UpVector);
			vec2 uvSun;
			LutTransmittanceParamsToUv(pHeight, SunZenithCosAngle, uvSun);
			
			vec3 SunLuminanceInSpace = SunIlluminance / textureLod(TransmittanceLutTexture, uvUp, 0.0).rgb;
			return SunLuminanceInSpace * textureLod(TransmittanceLutTexture, uvSun, 0.0).rgb;
		}
	}
	return vec3(0.0);
}
#endif //TRANSMITTANCE_DISABLED

#define AP_KM_PER_SLICE 4.0
float AerialPerspectiveSliceToDepth(float slice) {
	return slice * AP_KM_PER_SLICE;
}

#if MULTIPLE_SCATTERING_ENABLED
vec3 GetMultipleScattering(vec3 scattering, vec3 extinction, vec3 worldPos, float viewZenithCosAngle) {
	uvec2 size = textureSize(MultiScatteringLutTexture, 0).xy;
	vec2 uv = clamp(vec2(viewZenithCosAngle*0.5 + 0.5, (length(worldPos) - Atmosphere.BottomRadius) / (Atmosphere.TopRadius - Atmosphere.BottomRadius)), 0.0, 1.0);
	uv = vec2(fromUnitToSubUvs(uv.x, size.x), fromUnitToSubUvs(uv.y, size.y));

	vec3 multiScatteredLuminance = textureLod(MultiScatteringLutTexture, uv, 0.0).rgb;
	return multiScatteredLuminance;
}
#endif

SingleScatteringResult IntegrateScatteredLuminance(vec3 WorldPos, vec3 WorldDir, vec3 SunDir,
bool ground, float SampleCountIni, bool VariableSampleCount, bool MieRayPhase, float tMaxMax,
vec3 SunIlluminance) {
	SingleScatteringResult result = {vec3(0), vec3(0), vec3(0), vec3(0), vec3(0), vec3(0)};

	// Compute next intersection with atmosphere or ground
	vec3 earthO = vec3(0.0, 0.0, 0.0);
	float tBottom = raySphereIntersectNearest(WorldPos, WorldDir, earthO, Atmosphere.BottomRadius);
	float tTop = raySphereIntersectNearest(WorldPos, WorldDir, earthO, Atmosphere.TopRadius);
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
	
	tMax = min(tMax, tMaxMax);
	
	// Sample count
	float SampleCount = SampleCountIni;
	float SampleCountFloor = SampleCountIni;
	float tMaxFloor = tMax;
	if (VariableSampleCount) {
		SampleCount = mix(RAYMARCH_MIN_SPP, RAYMARCH_MAX_SPP, clamp(tMax*0.01, 0.0, 1.0));
		SampleCountFloor = floor(SampleCount);
		tMaxFloor = tMax * SampleCountFloor / SampleCount; // rescale tMax to map to the last entire step segment.
	}
	float dt = tMax / SampleCount;

	// Phase functions
	const float uniformPhase = 1.0 / (4.0 * PI);
	const vec3 wi = SunDir;
	const vec3 wo = WorldDir;
	float cosTheta = dot(wi, wo);
	float MiePhaseValue = CornetteShanksMiePhaseFunction(Atmosphere.MiePhaseG, -cosTheta); // negate cosTheta due to WorldDir being a "in" direction.
	float RayleighPhaseValue = RayleighPhase(cosTheta);

	// When building the scattering factor, we assume light illuminance is 1 to compute a transfer function relative to identity illuminance of 1.
	// This make the scattering factor independent of the light. It is now only linked to the atmosphere properties.
	vec3 globalL = SunIlluminance;

	// Ray march the atmosphere to integrate optical depth
	vec3 L = vec3(0.0);
	vec3 throughput = vec3(1.0);
	vec3 OpticalDepth = vec3(0.0);
	float t = 0.0;
	float tPrev = 0.0;
	const float SampleSegmentT = 0.3;
	for (float s = 0.0; s < SampleCount; s += 1.0) {
		if (VariableSampleCount) {
			// More expensive but artifact-free
			float t0 = (s) / SampleCountFloor;
			float t1 = (s + 1.0f) / SampleCountFloor;
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
			t = t0 + (t1 - t0)*SampleSegmentT;
			dt = t1 - t0;
		} else {
			// Exact difference, important for accuracy of multiple scattering
			float NewT = tMax * (s + SampleSegmentT) / SampleCount;
			dt = NewT - t;
			t = NewT;
		}
		vec3 P = WorldPos + t * WorldDir;

		MediumSampleRGB medium = sampleMediumRGB(P);
		const vec3 SampleOpticalDepth = medium.extinction * dt;
		const vec3 SampleTransmittance = exp(-SampleOpticalDepth);
		OpticalDepth += SampleOpticalDepth;

		float pHeight = length(P);
		const vec3 UpVector = P / pHeight;
		float SunZenithCosAngle = dot(SunDir, UpVector);
		vec2 uv;
		LutTransmittanceParamsToUv(pHeight, SunZenithCosAngle, uv);
#ifndef TRANSMITTANCE_DISABLED
		vec3 TransmittanceToSun = textureLod(TransmittanceLutTexture, uv, 0.0).rgb;
#else
		vec3 TransmittanceToSun = vec3(0.0);
#endif

		vec3 PhaseTimesScattering;
		if (MieRayPhase)
			PhaseTimesScattering = medium.scatteringMie * MiePhaseValue + medium.scatteringRay * RayleighPhaseValue;
		else
			PhaseTimesScattering = medium.scattering * uniformPhase;

		// Earth shadow
		float tEarth = raySphereIntersectNearest(P, SunDir, earthO + PLANET_RADIUS_OFFSET * UpVector, Atmosphere.BottomRadius);
		float earthShadow = tEarth >= 0.0 ? 0.0 : 1.0;

		// Dual scattering for multi scattering

		vec3 multiScatteredLuminance = vec3(0.0);
#if MULTIPLE_SCATTERING_ENABLED
		multiScatteredLuminance = GetMultipleScattering(medium.scattering, medium.extinction, P, SunZenithCosAngle);
#endif

		vec3 S = globalL * (earthShadow * TransmittanceToSun * PhaseTimesScattering + multiScatteredLuminance * medium.scattering);

		// When using the power serie to accumulate all scattering order, serie r must be <1 for a serie to converge.
		// Under extreme coefficient, MultiScatAs1 can grow larger and thus result in broken visuals.
		// The way to fix that is to use a proper analytical integration as proposed in slide 28 of http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/
		// However, it is possible to disable as it can also work using simple power serie sum unroll up to 5th order. The rest of the orders has a really low contribution.

		vec3 MS = medium.scattering * 1;
		vec3 MSint = (MS - MS * SampleTransmittance) / medium.extinction;
		result.MultiScatAs1 += throughput * MSint;

		// Evaluate input to multi scattering
		vec3 newMS;

		newMS = earthShadow * TransmittanceToSun * medium.scattering * uniformPhase * 1;
		result.NewMultiScatStep0Out += throughput * (newMS - newMS * SampleTransmittance) / medium.extinction;

		newMS = medium.scattering * uniformPhase * multiScatteredLuminance;
		result.NewMultiScatStep1Out += throughput * (newMS - newMS * SampleTransmittance) / medium.extinction;

		// See slide 28 at http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/
		vec3 Sint = (S - S * SampleTransmittance) / medium.extinction; // integrate along the current step segment
		L += throughput * Sint; // accumulate and also take into account the transmittance from previous steps
		throughput *= SampleTransmittance;

		tPrev = t;
	}

	if (ground && tMax == tBottom && tBottom > 0.0) {
		// Account for bounced light off the earth
		vec3 P = WorldPos + tBottom * WorldDir;
		float pHeight = length(P);

		const vec3 UpVector = P / pHeight;
		float SunZenithCosAngle = dot(SunDir, UpVector);
		vec2 uv;
		LutTransmittanceParamsToUv(pHeight, SunZenithCosAngle, uv);
#ifndef TRANSMITTANCE_DISABLED
		vec3 TransmittanceToSun = textureLod(TransmittanceLutTexture, uv, 0.0).rgb;
#else
		vec3 TransmittanceToSun = vec3(0.0);
#endif

		const float NdotL = clamp(dot(normalize(UpVector), normalize(SunDir)), 0.0, 1.0);
		L += globalL * TransmittanceToSun * throughput * NdotL * Atmosphere.GroundAlbedo / PI;
	}

	result.L = L;
	result.OpticalDepth = OpticalDepth;
	result.Transmittance = throughput;
	return result;
}

bool MoveToTopAtmosphere(inout vec3 WorldPos, in vec3 WorldDir, in float AtmosphereTopRadius) {
	float viewHeight = length(WorldPos);
	if (viewHeight > AtmosphereTopRadius) {
		float tTop = raySphereIntersectNearest(WorldPos, WorldDir, vec3(0.0), AtmosphereTopRadius);
		if (tTop >= 0.0) {
			vec3 UpVector = WorldPos / viewHeight;
			vec3 UpOffset = UpVector * -PLANET_RADIUS_OFFSET;
			WorldPos = WorldPos + WorldDir * tTop + UpOffset;
		} else {
			// Ray is not intersecting the atmosphere
			return false;
		}
	}
	return true; // ok to start tracing
}

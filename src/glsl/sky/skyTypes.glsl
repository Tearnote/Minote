#ifndef SKYTYPES_GLSL
#define SKYTYPES_GLSL

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

#endif //SKYTYPES_GLSL

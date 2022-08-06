#ifndef SKYTYPES_GLSL
#define SKYTYPES_GLSL

struct SingleScatteringResult {
	
	float3 L; // Scattered light (luminance)
	float3 opticalDepth; // Optical depth (1/m)
	float3 transmittance; // Transmittance in [0,1] (unitless)
	float3 multiScatAs1;
	
	float3 newMultiScatStep0Out;
	float3 newMultiScatStep1Out;
	
};

struct AtmosphereParams {
	
	float bottomRadius; // Radius of the planet (center to ground)
	float topRadius; // Maximum considered atmosphere height (center to atmosphere top)
	
	float rayleighDensityExpScale; // Rayleigh scattering exponential distribution scale in the atmosphere
	float pad0;
	float3 rayleighScattering; // Rayleigh scattering coefficients
	
	float mieDensityExpScale; // Mie scattering exponential distribution scale in the atmosphere
	float3 mieScattering; // Mie scattering coefficients
	float pad1;
	float3 mieExtinction; // Mie extinction coefficients
	float pad2;
	float3 mieAbsorption; // Mie absorption coefficients
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
	float3 absorptionExtinction; // This other medium only absorb light, e.g. useful to represent ozone in the earth atmosphere
	float pad6;
	
	float3 groundAlbedo; // The albedo of the ground.
	
};

#endif //SKYTYPES_GLSL

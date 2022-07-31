#ifndef SKY_TYPES_HLSL
#define SKY_TYPES_HLSL

struct AtmosphereParams {
	float bottomRadius;
	float topRadius;
	
	float rayleighDensityExpScale;
	float pad0;
	float3 rayleighScattering;
	
	float mieDensityExpScale;
	float3 mieScattering;
	float pad1;
	float3 mieExtinction;
	float pad2;
	float3 mieAbsorption;
	float miePhaseG;
	
	float absorptionDensity0LayerWidth;
	float absorptionDensity0ConstantTerm;
	float absorptionDensity0LinearTerm;
	float absorptionDensity1ConstantTerm;
	float absorptionDensity1LinearTerm;
	float pad3;
	float pad4;
	float pad5;
	float3 absorptionExtinction;
	float pad6;
	
	float3 groundAlbedo;
};

#endif

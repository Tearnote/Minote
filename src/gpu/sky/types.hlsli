#ifndef SKY_TYPES_HLSLI
#define SKY_TYPES_HLSLI

struct AtmosphereParams {
	float bottomRadius;
	float topRadius;
	
	float rayleighDensityExpScale;
	float _pad0;
	float3 rayleighScattering;
	
	float mieDensityExpScale;
	float3 mieScattering;
	float _pad1;
	float3 mieExtinction;
	float _pad2;
	float3 mieAbsorption;
	float miePhaseG;
	
	float absorptionDensity0LayerWidth;
	float absorptionDensity0ConstantTerm;
	float absorptionDensity0LinearTerm;
	float absorptionDensity1ConstantTerm;
	float absorptionDensity1LinearTerm;
	float _pad3;
	float _pad4;
	float _pad5;
	float3 absorptionExtinction;
	float _pad6;
	
	float3 groundAlbedo;
};

#endif

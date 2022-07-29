struct Params {
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

[[vk::binding(0)]]
StructuredBuffer<Params> params;

[[vk::binding(1)]]
Texture2D<float4> transmittance;

[[vk::binding(2)]]
RWTexture2D<float4> multiScattering;

[numthreads(8, 8, 1)]
void main(uint3 tid: SV_DispatchThreadID) {
	
	float4 val = transmittance[tid.xy];
	multiScattering[tid.xy] = float4(val.xyz, params[0].pad1);
	
}

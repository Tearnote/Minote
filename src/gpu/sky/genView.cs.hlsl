#include "sky/types.hlsl"
#include "world.hlsl"

[[vk::binding(0)]] ConstantBuffer<World> c_world;
[[vk::binding(1)]] ConstantBuffer<AtmosphereParams> c_params;
[[vk::binding(2)]][[vk::combinedImageSampler]] Texture2D<float4> s_transmittance;
[[vk::binding(2)]][[vk::combinedImageSampler]] SamplerState s_transmittanceSampler;
[[vk::binding(3)]][[vk::combinedImageSampler]] Texture2D<float4> s_multiScattering;
[[vk::binding(3)]][[vk::combinedImageSampler]] SamplerState s_multiScatteringSampler;
[[vk::binding(4)]] RWTexture2D<float3> t_view;

#define TRANSMITTANCE_TEX s_transmittance
#define TRANSMITTANCE_SAMPLER s_transmittanceSampler
#define MULTI_SCATTERING_TEX s_multiScattering
#define MULTI_SCATTERING_SAMPLER s_multiScatteringSampler
#include "sky/func.hlsl"

struct Constants {
	float3 probePos;
};
[[vk::push_constant]] Constants c_constants;

[[vk::constant_id(0)]] const uint ViewWidth = 0;
[[vk::constant_id(1)]] const uint ViewHeight = 0;
static const uint2 ViewSize = {ViewWidth, ViewHeight};

[numthreads(8, 8, 1)]
void main(uint3 _tid: SV_DispatchThreadID) {
	
	if (any(_tid.xy >= ViewSize))
		return;
	
	//TODO which?
	// float2 pixPos = float2(_tid.xy) + 0.5;
	float2 pixPos = float2(_tid.xy);
	float3 worldPos = c_constants.probePos + float3(0, 0, c_params.bottomRadius);
	float2 uv = pixPos / float2(ViewSize);
	
	float viewHeight = length(worldPos);
	
	float viewZenithCosAngle;
	float lightViewCosAngle;
	uvToSkyViewLutParams(viewZenithCosAngle, lightViewCosAngle, c_params, ViewSize, viewHeight, uv);
	
	float3 upVector = worldPos / viewHeight;
	float sunZenithCosAngle = dot(upVector, c_world.sunDirection);
	float3 sunDir = normalize(float3(sqrt(1.0 - sunZenithCosAngle * sunZenithCosAngle), 0.0, sunZenithCosAngle));
	
	worldPos = float3(0.0, 0.0, viewHeight);
	
	float viewZenithSinAngle = sqrt(1.0 - viewZenithCosAngle * viewZenithCosAngle);
	float3 worldDir = {
		viewZenithSinAngle * lightViewCosAngle,
		viewZenithSinAngle * sqrt(1.0 - lightViewCosAngle * lightViewCosAngle),
		viewZenithCosAngle
	};
	
	if (!moveToTopAtmosphere(worldPos, worldDir, c_params.topRadius)) {
		// Ray is not intersecting the atmosphere
		t_view[_tid.xy] = float3(0.0, 0.0, 0.0);
		return;
	}
	
	bool ground = false;
	float sampleCountIni = 30;
	bool variableSampleCount = true;
	bool mieRayPhase = true;
	SingleScatteringResult result = integrateScatteredLuminance(c_params, worldPos, worldDir, sunDir,
		ground, sampleCountIni, variableSampleCount, mieRayPhase, 9000000.0, c_world.sunIlluminance);
	
	t_view[_tid.xy] = result.L;
	
}

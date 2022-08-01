#include "sky/types.hlsl"
#include "world.hlsl"

[[vk::binding(0)]] ConstantBuffer<World> c_world;
[[vk::binding(1)]] ConstantBuffer<AtmosphereParams> c_params;
[[vk::binding(2)]][[vk::combinedImageSampler]] Texture2D<float4> s_transmittance;
[[vk::binding(2)]][[vk::combinedImageSampler]] SamplerState s_transmittanceSampler;
[[vk::binding(3)]][[vk::combinedImageSampler]] Texture2D<float4> s_multiScattering;
[[vk::binding(3)]][[vk::combinedImageSampler]] SamplerState s_multiScatteringSampler;
[[vk::binding(4)]] RWTexture2D<float3> t_view;

struct Constants {
	float3 probePos;
};
[[vk::push_constant]] Constants c_constants;

[[vk::constant_id(0)]] const uint ViewWidth = 0;
[[vk::constant_id(1)]] const uint ViewHeight = 0;
static const uint2 ViewSize = {ViewWidth, ViewHeight};

[numthreads(8, 8, 1)]
void main(uint3 _tid: SV_DispatchThreadID) {
	
	
	
}

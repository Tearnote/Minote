#include "visibility/worklist.hlsli"
#include "sky/access.hlsli"
#include "sky/types.hlsli"

struct Constants {
	float4x4 viewProjectionInv;
	float3 cameraPos;
	float _pad0;
	float3 sunDirection;
	float _pad1;
	float3 sunIlluminance;
};

[[vk::binding(0)]] ConstantBuffer<AtmosphereParams> c_params;
[[vk::binding(1)]][[vk::combinedImageSampler]] Texture2D<float4> s_transmittance;
[[vk::binding(1)]][[vk::combinedImageSampler]] SamplerState s_transmittanceSmp;
[[vk::binding(2)]][[vk::combinedImageSampler]] Texture2D<float3> s_view;
[[vk::binding(2)]][[vk::combinedImageSampler]] SamplerState s_viewSmp;
[[vk::binding(3)]] StructuredBuffer<uint> b_lists;
[[vk::binding(4)]] RWTexture2D<float4> t_target;

#define TRANSMITTANCE_TEX s_transmittance
#define TRANSMITTANCE_SMP s_transmittanceSmp
#include "sky/func.hlsli"

[[vk::constant_id(0)]] const uint ViewWidth = 0;
[[vk::constant_id(1)]] const uint ViewHeight = 0;
static const uint2 ViewSize = {ViewWidth, ViewHeight};
[[vk::constant_id(2)]] const uint TargetWidth = 0;
[[vk::constant_id(3)]] const uint TargetHeight = 0;
static const uint2 TargetSize = {TargetWidth, TargetHeight};
[[vk::constant_id(4)]] const uint TileOffset = 0;

[[vk::push_constant]] Constants C;

[numthreads(1, TileSize, TileSize)]
void main(uint3 _tid: SV_DispatchThreadID) {
	
	_tid.xy = tiledPos(b_lists, _tid, TileOffset);
	if (any(_tid.xy >= TargetSize))
		return;
	
	float2 gUv = (float2(_tid.xy) + 0.5) / float2(TargetSize);
	float3 clipSpace = {gUv * 2.0 - 1.0, 0.0};
	float4 hPos = mul(float4(clipSpace, 1.0), C.viewProjectionInv); //TODO broken
	
	float3 worldDir = normalize(hPos.xyz);
	float3 worldPos = C.cameraPos + float3(0.0, 0.0, c_params.bottomRadius);
	
	float viewHeight = length(worldPos);
	
	float3 upVector = normalize(worldPos);
	float viewZenithCosAngle = dot(worldDir, upVector);
	
	float3 sideVector = normalize(cross(upVector, worldDir)); // assumes non parallel vectors
	float3 forwardVector = normalize(cross(sideVector, upVector)); // aligns toward the sun light but perpendicular to up vector
	float2 lightOnPlane = {dot(C.sunDirection, forwardVector), dot(C.sunDirection, sideVector)};
	lightOnPlane = normalize(lightOnPlane);
	float lightViewCosAngle = lightOnPlane.x;
	
	bool intersectGround = raySphereIntersectNearest(worldPos, worldDir, float3(0.0, 0.0, 0.0), c_params.bottomRadius) >= 0.0;
	
	float2 uv;
	skyViewLutParamsToUv(uv, intersectGround, viewZenithCosAngle, lightViewCosAngle,
		ViewSize, viewHeight, c_params.bottomRadius);
	float3 skyView = s_view.SampleLevel(s_viewSmp, uv, 0);
	float3 sun = getSunLuminance(c_params, worldPos, worldDir, C.sunDirection, C.sunIlluminance)
		* (120000.0 / C.sunIlluminance);
	
	t_target[_tid.xy] = float4(skyView + sun, 1.0);
	
}

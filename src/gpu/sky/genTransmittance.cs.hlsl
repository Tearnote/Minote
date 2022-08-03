#include "sky/access.hlsli"
#include "sky/types.hlsli"
#include "sky/func.hlsli"

[[vk::binding(0)]] ConstantBuffer<AtmosphereParams> c_params;
[[vk::binding(1)]] RWTexture2D<float4> t_transmittance;

[[vk::constant_id(0)]] const uint TransmittanceWidth = 0;
[[vk::constant_id(1)]] const uint TransmittanceHeight = 0;
static const uint2 TransmittanceSize = {TransmittanceWidth, TransmittanceHeight};

[numthreads(8, 8, 1)]
void main(uint3 _tid: SV_DispatchThreadID) {
	
	if (any(_tid.xy >= TransmittanceSize))
		return;
	
	// Compute camera position from LUT coords
	float2 uv = (float2(_tid.xy) + 0.5) / float2(TransmittanceSize);
	float viewHeight;
	float viewZenithCosAngle;
	uvToLutTransmittanceParams(viewHeight, viewZenithCosAngle, c_params, uv);
	
	float3 worldPos = float3(0.0, 0.0, viewHeight);
	float3 worldDir = float3(0.0, sqrt(1.0 - viewZenithCosAngle * viewZenithCosAngle), viewZenithCosAngle);
	
	bool ground = false;
	float sampleCountIni = 40.0; // Can go as low as 10 sample but energy lost starts to be visible.
	bool variableSampleCount = false;
	bool mieRayPhase = false;
	float3 result = exp(-integrateScatteredLuminance(c_params, worldPos, worldDir,
		float3(1.0, 1.0, 1.0)/*unused*/, ground, sampleCountIni, variableSampleCount,
		mieRayPhase, 9000000.0, float3(1.0, 1.0, 1.0)).opticalDepth);
	
	// Optical depth to transmittance
	t_transmittance[_tid.xy] = float4(result, 1.0);
	
}

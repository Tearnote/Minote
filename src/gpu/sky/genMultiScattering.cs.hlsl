#include "types.hlsl"

[[vk::binding(0)]]
ConstantBuffer<AtmosphereParams> params;

[[vk::binding(1)]]
Texture2D<float4> transmittance;

[[vk::binding(2)]]
RWTexture2D<float4> multiScattering;

[numthreads(8, 8, 1)]
void main(uint3 tid: SV_DispatchThreadID) {
	
	
	
}

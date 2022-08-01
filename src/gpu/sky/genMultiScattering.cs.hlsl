#include "sky/access.hlsl"
#include "sky/types.hlsl"

[[vk::binding(0)]] ConstantBuffer<AtmosphereParams> c_params;
[[vk::binding(1)]][[vk::combinedImageSampler]] Texture2D<float4> s_transmittance;
[[vk::binding(1)]][[vk::combinedImageSampler]] SamplerState s_transmittanceSampler;
[[vk::binding(2)]] RWTexture2D<float4> t_multiScattering;

#define TRANSMITTANCE_TEX s_transmittance
#define TRANSMITTANCE_SAMPLER s_transmittanceSampler
#include "sky/func.hlsl"

[[vk::constant_id(0)]] const uint MultiScatteringWidth = 0;
[[vk::constant_id(1)]] const uint MultiScatteringHeight = 0;
static const uint2 MultiScatteringSize = {MultiScatteringWidth, MultiScatteringHeight};

static const uint SqrtSampleCount = 8;

groupshared float3 sh_multiScatAs1[64];
groupshared float3 sh_L[64];

[numthreads(1, 1, 64)]
void main(uint3 _tid: SV_DispatchThreadID) {
	
	if (any(_tid.xy >= MultiScatteringSize))
		return;
	
	// Compute camera position from LUT coords
	float2 uv = (float2(_tid.xy) + 0.5) / float2(MultiScatteringSize);
	uv = float2(fromSubUvsToUnit(uv.x, MultiScatteringSize.x), fromSubUvsToUnit(uv.y, MultiScatteringSize.y));
	
	float cosSunZenithAngle = uv.x * 2.0 - 1.0;
	float3 sunDir = {0.0, sqrt(saturate(1.0 - cosSunZenithAngle * cosSunZenithAngle)), cosSunZenithAngle};
	// We adjust again viewHeight according to PlanetRadiusOffset to be in a valid range.
	float viewHeight = c_params.bottomRadius +
		saturate(uv.y + PlanetRadiusOffset) * (c_params.topRadius - c_params.bottomRadius - PlanetRadiusOffset);
	
	float3 worldPos = {0.0, 0.0, viewHeight};
	float3 worldDir = {0.0, 0.0, 1.0};
	
	bool ground = true;
	float sampleCountIni = 20; // a minimum set of step is required for accuracy unfortunately
	bool variableSampleCount = false;
	bool mieRayPhase = false;
	
	float sphereSolidAngle = 4.0 * PI;
	float isotropicPhase = 1.0 / sphereSolidAngle;
	
	// Reference. Since there are many samples, it requires MULTI_SCATTERING_POWER_SERIE
	// to be true for accuracy and to avoid divergences (see declaration for explanations)
	float sqrtSample = SqrtSampleCount;
	float i = 0.5 + float(_tid.z / SqrtSampleCount);
	float j = 0.5 + float(_tid.z - float((_tid.z / SqrtSampleCount) * SqrtSampleCount));
	{
		float randA = i / sqrtSample;
		float randB = j / sqrtSample;
		float theta = 2.0 * PI * randA;
		float phi = PI * randB;
		float cosPhi = cos(phi);
		float sinPhi = sin(phi);
		float cosTheta = cos(theta);
		float sinTheta = sin(theta);
		worldDir = float3(cosTheta * sinPhi, sinTheta * sinPhi, cosPhi);
		SingleScatteringResult result = integrateScatteredLuminance(c_params, worldPos, worldDir, sunDir,
			ground, sampleCountIni, variableSampleCount, mieRayPhase, 9000000.0, float3(1.0, 1.0, 1.0));
		
		sh_multiScatAs1[_tid.z] = result.multiScatAs1 * sphereSolidAngle / (sqrtSample * sqrtSample);
		sh_L[_tid.z] = result.L * sphereSolidAngle / (sqrtSample * sqrtSample);
	}
	
	GroupMemoryBarrierWithGroupSync();
	
	// 64 to 32
	if (_tid.z < 32) {
		sh_multiScatAs1[_tid.z] += sh_multiScatAs1[_tid.z + 32];
		sh_L[_tid.z] += sh_L[_tid.z + 32];
	}
	GroupMemoryBarrierWithGroupSync();
	// 32 to 16
	if (_tid.z < 16) {
		sh_multiScatAs1[_tid.z] += sh_multiScatAs1[_tid.z + 16];
		sh_L[_tid.z] += sh_L[_tid.z + 16];
	}
	GroupMemoryBarrierWithGroupSync();
	// 16 to 8
	if (_tid.z < 8) {
		sh_multiScatAs1[_tid.z] += sh_multiScatAs1[_tid.z + 8];
		sh_L[_tid.z] += sh_L[_tid.z + 8];
	}
	GroupMemoryBarrierWithGroupSync();
	if (_tid.z < 4) {
		sh_multiScatAs1[_tid.z] += sh_multiScatAs1[_tid.z + 4];
		sh_L[_tid.z] += sh_L[_tid.z + 4];
	}
	GroupMemoryBarrierWithGroupSync();
	if (_tid.z < 2) {
		sh_multiScatAs1[_tid.z] += sh_multiScatAs1[_tid.z + 2];
		sh_L[_tid.z] += sh_L[_tid.z + 2];
	}
	GroupMemoryBarrierWithGroupSync();
	if (_tid.z < 1) {
		sh_multiScatAs1[_tid.z] += sh_multiScatAs1[_tid.z + 1];
		sh_L[_tid.z] += sh_L[_tid.z + 1];
	}
	GroupMemoryBarrierWithGroupSync();
	
	if (_tid.z > 0)
		return;
	
	float3 multiScatAs1 = sh_multiScatAs1[0] * isotropicPhase; // Equation 7 f_ms
	float3 inScatteredLuminance = sh_L[0] * isotropicPhase;    // Equation 5 L_2ndOrder
	
	// multiScatAs1 represents the amount of luminance scattered as if the integral
	// of scattered luminance over the sphere would be 1.
	//  - 1st order of scattering: one can ray-march a straight path as usual over
	// the sphere. That is inScatteredLuminance.
	//  - 2nd order of scattering: the inscattered luminance is inScatteredLuminance
	// at each of samples of fist order integration. Assuming a uniform phase function
	// that is represented by multiScatAs1,
	//  - 3nd order of scattering: the inscattered luminance is
	// (inScatteredLuminance * multiScatAs1 * multiScatAs1)
	//  - etc.
	// For a serie, sum_{n=0}^{n=+inf} = 1 + r + r^2 + r^3 + ... + r^n = 1 / (1.0 - r),
	// see https://en.wikipedia.org/wiki/Geometric_series
	float3 r = multiScatAs1;
	float3 sumOfAllMultiScatteringEventsContribution = rcp(1.0 - r);
	float3 L = inScatteredLuminance * sumOfAllMultiScatteringEventsContribution; // Equation 10 Psi_ms
	
	t_multiScattering[_tid.xy] = float4(L, 1.0);
	
}

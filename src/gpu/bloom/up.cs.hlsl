[[vk::binding(0)]][[vk::combinedImageSampler]] Texture2D<float4> s_source;
[[vk::binding(0)]][[vk::combinedImageSampler]] SamplerState s_sourceSmp;
[[vk::binding(1)]] RWTexture2D<float4> t_target;

[[vk::constant_id(0)]] const uint SourceWidth = 0;
[[vk::constant_id(1)]] const uint SourceHeight = 0;
static const uint2 SourceSize = {SourceWidth, SourceHeight};
[[vk::constant_id(2)]] const uint TargetWidth = 0;
[[vk::constant_id(3)]] const uint TargetHeight = 0;
static const uint2 TargetSize = {TargetWidth, TargetHeight};

struct Constants {
	float power;
};
[[vk::push_constant]] Constants c_push;

[numthreads(8, 8, 1)]
void main(uint3 _tid: SV_DispatchThreadID) {
	
	if (any(_tid.xy >= TargetSize))
		return; // We can't return before all barriers are passed
	
	float2 uv = (float2(_tid.xy) + 0.5) / float2(TargetSize);
	float2 texel = rcp(float2(SourceSize));
	
	// Upscale fetches can't be cached in the same way as downscale
	// float4 current = t_target[_tid.xy];
	float4 result  = s_source.SampleLevel(s_sourceSmp, uv,                              0) * (4.0 / 16.0);
	       result += s_source.SampleLevel(s_sourceSmp, uv + float2(-texel.x,      0.0), 0) * (2.0 / 16.0);
	       result += s_source.SampleLevel(s_sourceSmp, uv + float2( texel.x,      0.0), 0) * (2.0 / 16.0);
	       result += s_source.SampleLevel(s_sourceSmp, uv + float2(     0.0, -texel.y), 0) * (2.0 / 16.0);
	       result += s_source.SampleLevel(s_sourceSmp, uv + float2(     0.0,  texel.y), 0) * (2.0 / 16.0);
	       result += s_source.SampleLevel(s_sourceSmp, uv + float2(-texel.x, -texel.y), 0) * (1.0 / 16.0);
	       result += s_source.SampleLevel(s_sourceSmp, uv + float2( texel.x, -texel.y), 0) * (1.0 / 16.0);
	       result += s_source.SampleLevel(s_sourceSmp, uv + float2(-texel.x,  texel.y), 0) * (1.0 / 16.0);
	       result += s_source.SampleLevel(s_sourceSmp, uv + float2( texel.x,  texel.y), 0) * (1.0 / 16.0);
	t_target[_tid.xy] += result * c_push.power;
	
}

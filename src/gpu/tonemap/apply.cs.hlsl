struct Constants {
	float4 term0;
	float4 term1;
	float4 term2;
	float4 term3;
};

[[vk::binding(0)]][[vk::combinedImageSampler]] Texture2D<float4> t_source;
[[vk::binding(0)]][[vk::combinedImageSampler]] SamplerState t_sourceSmp;
[[vk::binding(1)]] RWTexture2D<float4> t_target;

[[vk::constant_id(0)]] const uint TargetWidth = 0;
[[vk::constant_id(1)]] const uint TargetHeight = 0;
static const uint2 TargetSize = {TargetWidth, TargetHeight};

[[vk::push_constant]] Constants c_push;

float3 lottesTonemap(float3 _color, Constants _t) {
	
	float peak = max(_color.r, max(_color.g, _color.b));
	peak = max(peak, rcp(256.0 * 65536.0)); // Protect against /0
	float3 ratio = _color * rcp(peak);
	
	peak = pow(peak, _t.term0.x); // Contrast adjustment
	peak = peak / (pow(peak, _t.term0.y) * _t.term0.z + _t.term0.w); // Highlight compression
	
	// Convert to non-linear space and saturate
	// Saturation is folded into first transform
	ratio = pow(ratio, _t.term1.xyz);
	// Move towards white on overexposure
	float3 white = {1.0, 1.0, 1.0};
	ratio = lerp(ratio, white, pow(float3(peak, peak, peak), _t.term2.xyz));
	// Convert back to linear
	ratio = pow(ratio, _t.term3.xyz);
	
	return ratio * peak;
	
}

float3 srgbEncode(float3 _color) {
	
	return lerp(12.92 * _color, 1.055 * pow(_color, 1.0 / 2.4) - 0.055, _color >= 0.0031308);
	
}

[numthreads(8, 8, 1)]
void main(uint3 _tid: SV_DispatchThreadID) {
	
	if (any(_tid.xy >= TargetSize))
		return;
	
	float3 input = t_source[_tid.xy].rgb;
	float3 output = lottesTonemap(input, c_push);
	output = srgbEncode(output);
	t_target[_tid.xy] = float4(output, 1.0);
	
}

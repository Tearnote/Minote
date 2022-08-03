[[vk::binding(0)]][[vk::combinedImageSampler]] Texture2D<float4> s_source;
[[vk::binding(0)]][[vk::combinedImageSampler]] SamplerState s_sourceSmp;
[[vk::binding(1)]] RWTexture2D<float4> t_target;

[[vk::constant_id(0)]] const uint SourceWidth = 0;
[[vk::constant_id(1)]] const uint SourceHeight = 0;
static const uint2 SourceSize = {SourceWidth, SourceHeight};
[[vk::constant_id(2)]] const uint TargetWidth = 0;
[[vk::constant_id(3)]] const uint TargetHeight = 0;
static const uint2 TargetSize = {TargetWidth, TargetHeight};
[[vk::constant_id(4)]] const uint UseKaris = 0;

static const uint GroupSize = 16;

groupshared float4 sh_outer[GroupSize + 2][GroupSize + 2];
groupshared float4 sh_inner[GroupSize + 1][GroupSize + 1];

// https://github.com/hughsk/glsl-luma/blob/master/index.glsl
float luma(float3 _color) {
	
	float3 w = {0.299, 0.587, 0.114};
	return dot(_color, w);
	
}

// https://github.com/keijiro/KinoBloom/blob/master/Assets/Kino/Bloom/Shader/Bloom.cginc
float4 karisAverage(float4 _s1, float4 _s2, float4 _s3, float4 _s4) {
	
	float s1w = rcp(luma(_s1.rgb) + 1.0);
	float s2w = rcp(luma(_s2.rgb) + 1.0);
	float s3w = rcp(luma(_s3.rgb) + 1.0);
	float s4w = rcp(luma(_s4.rgb) + 1.0);
	float oneDivWsum = rcp(s1w + s2w + s3w + s4w);
	
	return (_s1 * s1w + _s2 * s2w + _s3 * s3w + _s4 * s4w) * oneDivWsum;
	
}

[numthreads(GroupSize, GroupSize, 1)]
void main(uint3 _tid: SV_DispatchThreadID, uint3 _lid: SV_GroupThreadID) {
	
	// Fill up the filter tap cache
	
	float2 uv = (float2(_tid.xy) + 0.5) / float2(TargetSize);
	float2 texel = rcp(float2(SourceSize));
	float2 texel2 = texel * 2.0;
	
	if (_lid.x <= 1)
		sh_outer[_lid.x  ][_lid.y+2] = s_source.SampleLevel(s_sourceSmp, uv + float2(-texel2.x, texel2.y), 0);
	if (_lid.y <= 1)
		sh_outer[_lid.x+2][_lid.y  ] = s_source.SampleLevel(s_sourceSmp, uv + float2(texel2.x, -texel2.y), 0);
	if (_lid.x <= 1 && _lid.y <= 1)
		sh_outer[_lid.x  ][_lid.y  ] = s_source.SampleLevel(s_sourceSmp, uv - texel2, 0);
	
	sh_outer[_lid.x+2][_lid.y+2] = s_source.SampleLevel(s_sourceSmp, uv + texel2, 0.0);
	
	if (_lid.x == 0)
		sh_inner[_lid.x  ][_lid.y+1] = s_source.SampleLevel(s_sourceSmp, uv + float2(-texel.x, texel.y), 0);
	if (_lid.y == 0)
		sh_inner[_lid.x+1][_lid.y  ] = s_source.SampleLevel(s_sourceSmp, uv + float2(texel.x, -texel.y), 0);
	if (_lid.x == 0 && _lid.y == 0)
		sh_inner[_lid.x  ][_lid.y  ] = s_source.SampleLevel(s_sourceSmp, uv - texel, 0);
	
	sh_inner[_lid.x+1][_lid.y+1] = s_source.SampleLevel(s_sourceSmp, uv + texel, 0);
	
	GroupMemoryBarrierWithGroupSync();
	
	if (any(_tid.xy >= TargetSize))
		return; // We can't return before all barriers are passed
	
	// Convolve source texture
	
	if (!UseKaris) {
		float4 resultInner =
			sh_inner[_lid.x  ][_lid.y  ] * (4.0 / 32.0) +
			sh_inner[_lid.x+1][_lid.y  ] * (4.0 / 32.0) +
			sh_inner[_lid.x  ][_lid.y+1] * (4.0 / 32.0) +
			sh_inner[_lid.x+1][_lid.y+1] * (4.0 / 32.0);
		float4 resultOuter =
			sh_outer[_lid.x  ][_lid.y  ] * (1.0 / 32.0) +
			sh_outer[_lid.x+1][_lid.y  ] * (2.0 / 32.0) +
			sh_outer[_lid.x+2][_lid.y  ] * (1.0 / 32.0) +
			sh_outer[_lid.x  ][_lid.y+1] * (2.0 / 32.0) +
			sh_outer[_lid.x+1][_lid.y+1] * (4.0 / 32.0) +
			sh_outer[_lid.x+2][_lid.y+1] * (2.0 / 32.0) +
			sh_outer[_lid.x  ][_lid.y+2] * (1.0 / 32.0) +
			sh_outer[_lid.x+1][_lid.y+2] * (2.0 / 32.0) +
			sh_outer[_lid.x+2][_lid.y+2] * (1.0 / 32.0);
		
		t_target[_tid.xy] = resultInner + resultOuter;
	} else {
		float4 resultInner = karisAverage(
			sh_inner[_lid.x  ][_lid.y  ],
			sh_inner[_lid.x+1][_lid.y  ],
			sh_inner[_lid.x  ][_lid.y+1],
			sh_inner[_lid.x+1][_lid.y+1]);
		
		float4 resultOuter1 = karisAverage(
			sh_outer[_lid.x  ][_lid.y  ],
			sh_outer[_lid.x+1][_lid.y  ],
			sh_outer[_lid.x  ][_lid.y+1],
			sh_outer[_lid.x+1][_lid.y+1]);
		float4 resultOuter2 = karisAverage(
			sh_outer[_lid.x+1][_lid.y  ],
			sh_outer[_lid.x+2][_lid.y  ],
			sh_outer[_lid.x+1][_lid.y+1],
			sh_outer[_lid.x+2][_lid.y+1]);
		float4 resultOuter3 = karisAverage(
			sh_outer[_lid.x  ][_lid.y+1],
			sh_outer[_lid.x+1][_lid.y+1],
			sh_outer[_lid.x  ][_lid.y+2],
			sh_outer[_lid.x+1][_lid.y+2]);
		float4 resultOuter4 = karisAverage(
			sh_outer[_lid.x+1][_lid.y+1],
			sh_outer[_lid.x+2][_lid.y+1],
			sh_outer[_lid.x+1][_lid.y+2],
			sh_outer[_lid.x+2][_lid.y+2]);
		
		float4 result =
			resultInner  * 0.5   +
			resultOuter1 * 0.125 +
			resultOuter2 * 0.125 +
			resultOuter3 * 0.125 +
			resultOuter4 * 0.125;
		
		t_target[_tid.xy] = result;
	}
	
}

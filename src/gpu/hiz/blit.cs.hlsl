[[vk::binding(0)]][[vk::combinedImageSampler]] Texture2D<float> t_source;
[[vk::binding(0)]][[vk::combinedImageSampler]] SamplerState t_sourceSmp;
[[vk::binding(1)]] RWTexture2D<float> t_hiz;

[[vk::constant_id(0)]] const uint SourceWidth = 0;
[[vk::constant_id(1)]] const uint SourceHeight = 0;
static const uint2 SourceSize = {SourceWidth, SourceHeight};
[[vk::constant_id(2)]] const uint HiZSize = 0;

static const uint2 GatherOffsets[4] = {
	{0, 1},
	{1, 1},
	{1, 0},
	{0, 0},
};

[numthreads(8, 8, 1)]
void main(uint3 _tid: SV_DispatchThreadID) {
	
	_tid.xy *= 2;
	if (any(_tid.xy >= SourceSize))
		return;
	
	// Gather 4 input values
	const uint2 HizOffset = (HiZSize - SourceSize) / 2;
	const float2 SourcePitch = rcp(float2(SourceSize));
	float4 depthSamples = t_source.Gather(t_sourceSmp, float2(_tid.xy + 1) * SourcePitch);
	
	// Write out to destination
	uint2 hizPos = HizOffset + _tid.xy;
	for (uint i = 0; i < 4; i += 1)
		t_hiz[hizPos + GatherOffsets[i]] = depthSamples[i];
	
}

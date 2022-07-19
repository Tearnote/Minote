struct VSOutput {
	float4 position: SV_POSITION;
	[[vk::location(0)]] float4 color: COLOR;
	[[vk::location(1)]] float2 uv: TEXCOORD;
};

[[vk::combinedImageSampler]][[vk::binding(0)]] Texture2D texture;
[[vk::combinedImageSampler]][[vk::binding(0)]] SamplerState textureSampler;

float4 main(VSOutput input): SV_TARGET {
	
	return input.color * texture.Sample(textureSampler, input.uv);
	
}

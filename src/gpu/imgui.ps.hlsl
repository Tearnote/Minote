struct VSOutput {
	float4 position: SV_POSITION;
	float4 color: COLOR;
	float2 uv: TEXCOORD;
};

[[vk::combinedImageSampler]][[vk::binding(0)]] Texture2D texture;
[[vk::combinedImageSampler]][[vk::binding(0)]] SamplerState textureSampler;

float4 main(VSOutput input): SV_TARGET {
	
	return input.color * texture.Sample(textureSampler, input.uv);
	
}

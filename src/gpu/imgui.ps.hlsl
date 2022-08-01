struct VSOutput {
	float4 position: SV_POSITION;
	float4 color: COLOR;
	float2 uv: TEXCOORD;
};

[[vk::binding(0)]][[vk::combinedImageSampler]] Texture2D texture;
[[vk::binding(0)]][[vk::combinedImageSampler]] SamplerState textureSampler;

float4 main(VSOutput _input): SV_TARGET {
	
	return _input.color * texture.Sample(textureSampler, _input.uv);
	
}

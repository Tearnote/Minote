struct VSOutput {
	float4 position: SV_POSITION;
	float4 color: COLOR;
	float2 uv: TEXCOORD;
};

[[vk::binding(0)]][[vk::combinedImageSampler]] Texture2D s_texture;
[[vk::binding(0)]][[vk::combinedImageSampler]] SamplerState s_textureSmp;

float4 main(VSOutput _input): SV_TARGET {
	
	return _input.color * s_texture.Sample(s_textureSmp, _input.uv);
	
}

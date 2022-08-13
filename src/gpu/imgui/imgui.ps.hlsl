struct VSOutput {
	float4 position: SV_Position;
	float4 color: Color;
	float2 uv: TexCoord;
};

[[vk::binding(0)]][[vk::combinedImageSampler]] Texture2D s_texture;
[[vk::binding(0)]][[vk::combinedImageSampler]] SamplerState s_textureSmp;

float4 main(VSOutput _input): SV_Target {
	
	return _input.color * s_texture.Sample(s_textureSmp, _input.uv);
	
}

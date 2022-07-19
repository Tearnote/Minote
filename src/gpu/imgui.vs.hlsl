struct VSInput {
	[[vk::location(0)]] float2 position: POSITION;
	[[vk::location(1)]] float2 uv: TEXCOORD;
	[[vk::location(2)]] float4 color: COLOR;
};

struct VSOutput {
	float4 position: SV_POSITION;
	[[vk::location(0)]] float4 color: COLOR;
	[[vk::location(1)]] float2 uv: TEXCOORD;
};

struct Constants {
	float2 scale;
	float2 translate;
};
[[vk::push_constant]] Constants constants;

VSOutput main(VSInput input) {
	
	VSOutput output;
	
	output.color = input.color;
	output.uv = input.uv;
	output.position = float4(input.position * constants.scale + constants.translate, 0.0, 1.0);
	
	return output;
	
}

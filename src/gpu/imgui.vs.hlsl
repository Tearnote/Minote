struct VSInput {
	float2 position: POSITION;
	float2 uv: TEXCOORD;
	float4 color: COLOR;
};

struct VSOutput {
	float4 position: SV_POSITION;
	float4 color: COLOR;
	float2 uv: TEXCOORD;
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

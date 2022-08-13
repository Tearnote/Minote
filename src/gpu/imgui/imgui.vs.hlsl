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
[[vk::push_constant]] Constants c_push;

VSOutput main(VSInput _input) {
	
	VSOutput output;
	
	output.color = _input.color;
	output.uv = _input.uv;
	output.position = float4(_input.position * c_push.scale + c_push.translate, 0.0, 1.0);
	
	return output;
	
}

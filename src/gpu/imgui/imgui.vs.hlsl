struct Constants {
	float2 scale;
	float2 translate;
};

struct VSInput {
	float2 position: Position;
	float2 uv: TexCoord;
	float4 color: Color;
};

struct VSOutput {
	float4 position: SV_Position;
	float4 color: Color;
	float2 uv: TexCoord;
};

[[vk::push_constant]] Constants c_push;

VSOutput main(VSInput _input) {
	
	VSOutput output;
	
	output.color = _input.color;
	output.uv = _input.uv;
	output.position = float4(_input.position * c_push.scale + c_push.translate, 0.0, 1.0);
	
	return output;
	
}

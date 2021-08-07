#version 460
#pragma shader_stage(fragment)

layout(location = 0) in vec4 f_color;
layout(location = 1) in vec2 f_uv;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D s_texture;

void main() {
	
	out_color = f_color * texture(s_texture, f_uv.st);
	
}

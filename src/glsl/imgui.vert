#version 460
#pragma shader_stage(vertex)

layout(location = 0) in vec2 v_pos;
layout(location = 1) in vec2 v_uv;
layout(location = 2) in vec4 v_color;

layout(location = 0) out vec4 f_color;
layout(location = 1) out vec2 f_uv;

layout(push_constant) uniform PushConstants {
	vec2 u_scale;
	vec2 u_translate;
};

void main() {
	
	f_color = v_color;
	f_uv = v_uv;
	gl_Position = vec4(v_pos * u_scale + u_translate, 0.0, 1.0);
	
}

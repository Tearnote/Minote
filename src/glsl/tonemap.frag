#version 460
#pragma shader_stage(fragment)

layout(location = 0) in vec2 f_texCoords;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D source;
layout(binding = 1) uniform sampler2D bloom;

float luminance(vec3 v) {
	return dot(v, vec3(0.2126f, 0.7152f, 0.0722f));
}

vec3 reinhard_jodie(vec3 v) {
	float l = luminance(v);
	vec3 tv = v / (1.0f + v);
	return mix(v / (1.0f + l), tv, tv);
}

void main() {
	out_color = texture(source, f_texCoords) + texture(bloom, f_texCoords);
	out_color = vec4(reinhard_jodie(vec3(out_color)), 1.0);
}

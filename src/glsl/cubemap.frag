#version 460
#pragma shader_stage(fragment)

layout(location = 0) in vec4 f_position;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform World {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	vec4 lightPosition;
	vec4 lightColor;
	vec4 ambientColor;
} world;

layout(binding = 1) uniform sampler2D env;

#include "util.glslh"

void main() {
	vec2 uv = sampleSphericalMap(normalize(f_position.rgb));
	out_color = vec4(textureLod(env, uv, 0.0).rgb, 1.0);
}

#version 460

layout(location = 0) in vec2 f_texCoords;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D source;
layout(binding = 1) uniform sampler2D bloom;

void main() {
	out_color = texture(source, f_texCoords) + texture(bloom, f_texCoords);
}

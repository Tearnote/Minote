#version 460

layout(location = 0) in vec2 f_texCoords;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D source;

void main() {
	out_color = texture(source, f_texCoords);
}

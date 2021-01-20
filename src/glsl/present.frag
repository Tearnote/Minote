#version 460

layout(location = 0) in vec2 f_texCoords;

layout(location = 0) out vec4 out_color;

layout(input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput in_color;

void main() {
	out_color = subpassLoad(in_color);
}

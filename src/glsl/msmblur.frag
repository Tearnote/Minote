#version 460

layout(location = 0) in vec2 f_texCoords;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D source;

void main() {
	vec2 halfTexel = vec2(0.5) / vec2(textureSize(source, 0));
	out_color = texture(source, f_texCoords + halfTexel);
	out_color += texture(source, f_texCoords - halfTexel);
	halfTexel.x = -halfTexel.x;
	out_color += texture(source, f_texCoords + halfTexel);
	out_color += texture(source, f_texCoords - halfTexel);
	out_color /= 4.0;
}

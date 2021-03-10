#version 460

layout(location = 0) in vec2 f_texCoords;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D source;

layout(binding = 1) uniform StepSize {
	float stepSize;
};

void main() {
	vec2 texel = vec2(stepSize) / textureSize(source, 0);
	out_color = texture(source, f_texCoords + texel);
	out_color += texture(source, f_texCoords - texel);
	texel.x = -texel.x;
	out_color += texture(source, f_texCoords + texel);
	out_color += texture(source, f_texCoords - texel);
	out_color /= 4.0;
}

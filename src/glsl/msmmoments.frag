#version 460

layout(location = 0) in vec2 f_texCoords;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2DMS source;

vec4 Moments(float z) {
	const mat4 basis = mat4(
		1.5, 0.0, sqrt(3.0)/2.0, 0.0,
		0.0, 4.0, 0.0, 0.5,
		-2.0, 0.0, -sqrt(12.0)/9.0, 0.0,
		0.0, -4.0, 0.0, 0.5
	);
	return basis * vec4(z, pow(z, 2), pow(z, 3), pow(z, 4)) + vec4(0.5, 0.0, 0.5, 0.0);
}

void main() {
	ivec2 size = textureSize(source);
	vec2 uv = f_texCoords * vec2(size.x, size.y);
	ivec2 iuv = ivec2(uv.x, uv.y);

	out_color = (Moments(texelFetch(source, iuv, 0).r * 2.0 - 1.0) +
	             Moments(texelFetch(source, iuv, 1).r * 2.0 - 1.0) +
	             Moments(texelFetch(source, iuv, 2).r * 2.0 - 1.0) +
	             Moments(texelFetch(source, iuv, 3).r * 2.0 - 1.0)) / 4.0;
}

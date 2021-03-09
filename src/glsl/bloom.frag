#version 460

layout(location = 0) in vec2 f_texCoords;
layout(location = 1) in flat uint mode;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D source;

void main() {
	vec2 texel;

	switch (mode) {

	case 0: // Threshold
		const float threshold = 1.0;
		const float softKnee = 0.25;
		const float strength = 1.0;

		vec4 fragColor = texture(source, f_texCoords);
		float brightness = max(fragColor.r, max(fragColor.g, fragColor.b));
		float knee = threshold * softKnee;
		float soft = brightness - threshold + knee;
		soft = clamp(soft, 0, 2 * knee);
		soft = soft * soft / (4 * knee + 0.00001);
		float contribution = max(soft, brightness - threshold);
		contribution /= max(brightness, 0.00001);
		out_color = fragColor * contribution * strength;
		break;

	case 1: // Downscale
		texel = vec2(1.0) / textureSize(source, 0);
		out_color = vec4(0.0);
		out_color += texture(source, f_texCoords + texel) / 4.0;
		out_color += texture(source, f_texCoords - texel) / 4.0;
		texel.x = -texel.x;
		out_color += texture(source, f_texCoords + texel) / 4.0;
		out_color += texture(source, f_texCoords - texel) / 4.0;
		break;

	case 2: // Upscale
		texel = vec2(0.5) / textureSize(source, 0);
		out_color = vec4(0.0);
		out_color += texture(source, f_texCoords + texel) / 4.0;
		out_color += texture(source, f_texCoords - texel) / 4.0;
		texel.x = -texel.x;
		out_color += texture(source, f_texCoords + texel) / 4.0;
		out_color += texture(source, f_texCoords - texel) / 4.0;
		break;

	}
}

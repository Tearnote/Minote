#version 460

layout(location = 0) in vec2 f_texCoords;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D source;

void main() {
	const float threshold = 1.25;
	const float softKnee = 0.2;
	const float strength = 0.08;

	vec4 fragColor = texture(source, f_texCoords);
	float brightness = max(fragColor.r, max(fragColor.g, fragColor.b));
	float knee = threshold * softKnee;
	float soft = brightness - threshold + knee;
	soft = clamp(soft, 0, 2 * knee);
	soft = soft * soft / (4 * knee + 0.00001);
	float contribution = max(soft, brightness - threshold);
	contribution /= max(brightness, 0.00001);
	out_color = fragColor * contribution * strength;
}

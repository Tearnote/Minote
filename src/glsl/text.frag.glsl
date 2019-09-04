#version 330 core

in vec2 fTexCoords;

out vec4 outColor;

uniform sampler2D atlas;

const vec4 color = vec4(0.0, 0.0, 0.0, 1.0);

float median(float r, float g, float b) {
	return max(min(r, g), min(max(r, g), b));
}

void main()
{
	vec3 texel = texture2D(atlas, fTexCoords).rgb;
	float distance = median(texel.r, texel.g, texel.b) - 0.5;
	float d = fwidth(distance);
	float alpha = smoothstep(-d, d, distance);
	outColor = vec4(color.rgb, color.a * alpha);
}

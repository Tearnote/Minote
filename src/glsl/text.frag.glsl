#version 330 core

in vec2 fTexCoord;

out vec4 outColor;

uniform sampler2D atlas;

float median(float r, float g, float b) {
	return max(min(r, g), min(max(r, g), b));
}

void main()
{
	vec3 fragment = texture(atlas, fTexCoord).rgb;
	float sigDist = median(fragment.r, fragment.g, fragment.b);
	float w = fwidth(sigDist);
	float opacity = smoothstep(0.5 - w, 0.5 + w, sigDist);

	outColor = vec4(0.0f, 0.0f, 0.0f, opacity);
}

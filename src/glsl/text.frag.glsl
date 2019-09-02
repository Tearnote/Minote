#version 330 core

in vec2 fTexCoords;

out vec4 outColor;

uniform sampler2D atlas;

float pxRange = 4.0;
vec4 color = vec4(0.0, 0.0, 0.0, 1.0);

float median(float r, float g, float b) {
	return max(min(r, g), min(max(r, g), b));
}

void main()
{
	vec2 msdfUnit = pxRange / vec2(textureSize(atlas, 0));
	vec3 s = texture(atlas, fTexCoords).rgb;
	float sigDist = median(s.r, s.g, s.b) - 0.5;
	sigDist *= dot(msdfUnit, 0.5 / fwidth(fTexCoords));
	float opacity = clamp(sigDist + 0.5, 0.0, 1.0);

	outColor = mix(vec4(0.0, 0.0, 0.0, 0.0), color, opacity);
}
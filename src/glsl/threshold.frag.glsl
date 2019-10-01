#version 330 core

in vec2 fTexCoords;

out vec4 outColor;

uniform sampler2D screen;
uniform float threshold;

void main()
{
	vec4 fragColor = texture(screen, fTexCoords);
	float brightness = max(fragColor.r, max(fragColor.g, fragColor.b));
	float contribution = max(0, brightness - threshold);
	contribution /= max(brightness, 0.00001);
	outColor = fragColor * contribution;
}
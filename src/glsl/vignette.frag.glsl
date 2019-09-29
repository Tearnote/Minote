#version 330 core

in vec2 fTexCoords;

out vec4 outColor;

uniform float falloff;
uniform float aspect;

void main()
{
	vec2 coord = (fTexCoords - 0.5) * vec2(aspect, 1.0) * 2.0;
	float rf = sqrt(dot(coord, coord)) * falloff;
	float rf2_1 = rf * rf + 1.0;
	float e = 1.0 / (rf2_1 * rf2_1);
	outColor = vec4(0.0, 0.0, 0.0, 1.0 - e);
}
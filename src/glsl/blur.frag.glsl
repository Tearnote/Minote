#version 330 core

in vec2 fTexCoords;

out vec4 outColor;

uniform sampler2D screen;
uniform float step;

void main()
{
	vec2 step1 = vec2(step) / textureSize(screen, 0);
	vec4 color = vec4(0.0, 0.0, 0.0, 0.0);
	color += texture(screen, fTexCoords + step1) / 4.0;
	color += texture(screen,  fTexCoords - step1) / 4.0;
	vec2 step2 = step1;
	step2.x = -step2.x;
	color += texture(screen, fTexCoords + step2) / 4.0;
	color += texture(screen,  fTexCoords - step2) / 4.0;
	outColor = color;
}
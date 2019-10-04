#version 330 core

in vec4 fColor;

out vec4 outColor;

uniform float strength = 1.2;

void main()
{
	outColor = vec4(fColor.rgb * strength, fColor.a);
}

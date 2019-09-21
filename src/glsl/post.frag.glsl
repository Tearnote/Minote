#version 330 core

in vec2 fTexCoords;

out vec4 outColor;

uniform sampler2D screenTexture;

void main()
{
	outColor = texture(screenTexture, fTexCoords);
}
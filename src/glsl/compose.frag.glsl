// Minote - glsl/compose.frag.glsl

#version 330 core

in vec2 fTexCoords;

out vec4 outColor;

uniform sampler2D screen;
uniform sampler2D bloom;
uniform float bloomStrength;

void main()
{
	outColor = texture(screen, fTexCoords) +
	           texture(bloom, fTexCoords) * bloomStrength;
}
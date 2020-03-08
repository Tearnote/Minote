/**
 * Blit fragment shader
 * @file
 * Basic fullscreen blit function. Generates its own vertices, just draw
 * 3 vertices with no buffers attached.
 */

#version 330 core

in vec2 fTexCoords;

out vec4 outColor;

uniform sampler2D image;
uniform int layers;

void main()
{
	vec4 bloom = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	for (int i = 0; i < layers; i++)
		bloom += textureLod(image, fTexCoords, float(i));
    outColor = bloom;
}

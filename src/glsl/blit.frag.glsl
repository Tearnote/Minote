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
uniform float boost;

void main()
{
    outColor = texture(image, fTexCoords);
    outColor.rgb *= boost;
}

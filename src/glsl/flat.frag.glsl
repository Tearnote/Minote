/**
 * Flat shading fragment shader
 * @file
 * No lighting, just constant color + per-instance tint.
 */

#version 330 core

in vec4 fColor;

out vec4 outColor;

void main()
{
    outColor = fColor;
}

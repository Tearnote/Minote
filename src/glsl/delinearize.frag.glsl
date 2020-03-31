/**
 * Delinearize fragment shader
 * @file
 * Blits a texture with gamma correction. No vertices needed, render as a single
 * triangle.
 */

#version 330 core

in vec2 fTexCoords;

out vec4 outColor;

uniform sampler2D image;

#include "util.glslh"

void main()
{
    vec4 color = texture(image, fTexCoords);
    outColor = vec4(srgbEncode(color.rgb), color.a);
}

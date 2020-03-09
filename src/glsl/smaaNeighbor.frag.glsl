/**
 * SMAA neighborhood blending fragment shader
 * @file
 * Adapted from https://github.com/turol/smaaDemo
 */

#version 330 core

in vec2 fTexCoords;
in vec4 fOffset;

out vec4 outColor;

uniform sampler2D image1;
uniform sampler2D image2;
uniform sampler2D blend1;
uniform sampler2D blend2;
uniform vec4 screenSize;
uniform float alpha;

#define SMAA_RT_METRICS screenSize
#define SMAA_INCLUDE_VS 0
#define SMAA_INCLUDE_PS 1
#include "smaaParams.glslh"
#include "util.glslh"

void main()
{
    vec4 color1 = SMAANeighborhoodBlendingPS(fTexCoords, fOffset, image1, blend1);
    vec4 color2 = SMAANeighborhoodBlendingPS(fTexCoords, fOffset, image2, blend2);
    outColor = mix(color1, color2, 0.5);
}

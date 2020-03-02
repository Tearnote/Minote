/**
 * SMAA neighborhood blending fragment shader
 * @file
 * Adapted from https://github.com/turol/smaaDemo
 */

#version 330 core

in vec2 fTexCoords;
in vec4 fOffset;

out vec4 outColor;

uniform sampler2D image;
uniform sampler2D blend;
uniform vec4 screenSize;
uniform float alpha;

#define SMAA_RT_METRICS screenSize
#define SMAA_INCLUDE_VS 0
#define SMAA_INCLUDE_PS 1
#include "smaa.h"

void main()
{
    outColor = SMAANeighborhoodBlendingPS(fTexCoords, fOffset, image, blend);
    outColor *= alpha;
}

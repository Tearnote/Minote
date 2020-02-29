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

#define SMAA_RT_METRICS screenSize
#define SMAA_GLSL_3 1

#define SMAA_INCLUDE_VS 0
#define SMAA_INCLUDE_PS 1

#define SMAA_PRESET_ULTRA
#include "../../lib/smaa/smaa.h"

#define gamma 2.2

void main()
{
    outColor = SMAANeighborhoodBlendingPS(fTexCoords, fOffset, image, blend);
    outColor.rgb = pow(outColor.rgb, vec3(1.0 / gamma));
}

/**
 * SMAA blending weight calculation fragment shader
 * @file
 * Adapted from https://github.com/turol/smaaDemo
 */

#version 330 core

in vec2 fTexCoords;
in vec2 fPixCoords;
in vec4 fOffset[3];

out vec4 outColor;

uniform sampler2D edges;
uniform sampler2D area;
uniform sampler2D search;
uniform vec4 screenSize;

#define SMAA_RT_METRICS screenSize
#define SMAA_GLSL_3 1

#define SMAA_INCLUDE_VS 0
#define SMAA_INCLUDE_PS 1

#define SMAA_PRESET_ULTRA
#include "../../lib/smaa/smaa.h"

void main()
{
    outColor = SMAABlendingWeightCalculationPS(fTexCoords, fPixCoords, fOffset,
        edges, area, search, vec4(0.0));
}

/**
 * SMAA blending weight calculation fragment shader
 * @file
 * Adapted from https://github.com/turol/smaaDemo
 */

#version 330 core

in vec2 fTexCoords;
in vec2 fPixCoords;
in vec4 fOffset[3];

layout(location = 0) out vec4 outBlend1;
layout(location = 1) out vec4 outBlend2;

uniform sampler2D edges1;
uniform sampler2D edges2;
uniform sampler2D area;
uniform sampler2D search;
uniform vec4 screenSize;

#define SMAA_RT_METRICS screenSize
#define SMAA_INCLUDE_VS 0
#define SMAA_INCLUDE_PS 1
#include "smaaParams.glslh"

void main()
{
    outBlend1 = SMAABlendingWeightCalculationPS(fTexCoords, fPixCoords, fOffset,
        edges1, area, search, vec4(1, 2, 2, 0));
    outBlend2 = SMAABlendingWeightCalculationPS(fTexCoords, fPixCoords, fOffset,
        edges2, area, search, vec4(2, 1, 1, 0));
}

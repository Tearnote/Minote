// Minote - glsl/smaaBlend.frag.glsl
// SMAA blending weight calculation stage.
// Adapted from https://github.com/turol/smaaDemo

#version 330 core

in vec2 fTexCoords;
in vec2 fPixCoords;
in vec4 fOffset[3];

out vec4 outBlend;

uniform sampler2D edges;
uniform sampler2D area;
uniform sampler2D search;
uniform vec4 subsampleIndices;
uniform vec4 screenSize;

#define SMAA_RT_METRICS screenSize
#define SMAA_INCLUDE_VS 0
#define SMAA_INCLUDE_PS 1
#include "smaaParams.glslh"

void main()
{
    outBlend = SMAABlendingWeightCalculationPS(fTexCoords, fPixCoords, fOffset,
        edges, area, search, subsampleIndices);
}

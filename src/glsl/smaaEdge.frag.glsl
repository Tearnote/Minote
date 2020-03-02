/**
 * SMAA edge detection fragment shader
 * @file
 * Adapted from https://github.com/turol/smaaDemo
 */

#version 330 core

in vec2 fTexCoords;
in vec4 fOffset[3];

out vec4 outColor;

uniform sampler2D image;
uniform vec4 screenSize;

#define SMAA_RT_METRICS screenSize
#define SMAA_INCLUDE_VS 0
#define SMAA_INCLUDE_PS 1
#include "smaa.h"

void main()
{
    outColor = vec4(SMAAColorEdgeDetectionPS(fTexCoords, fOffset, image), 0.0, 0.0);
}

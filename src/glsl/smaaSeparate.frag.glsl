/**
 * SMAA multisample separation fragment shader
 * @file
 * Adapted from https://github.com/turol/smaaDemo
 */

#version 330 core

in vec2 fTexCoords;

layout(location = 0) out vec4 outColor1;
layout(location = 1) out vec4 outColor2;

uniform sampler2DMS image;
uniform vec4 screenSize;

#define SMAA_RT_METRICS screenSize
#define SMAA_GLSL_3 1

#define SMAA_INCLUDE_VS 0
#define SMAA_INCLUDE_PS 1

#define SMAA_PRESET_ULTRA
#include "../../lib/smaa/smaa.h"

void main()
{
    SMAASeparatePS(gl_FragCoord, fTexCoords, outColor1, outColor2, image);
}

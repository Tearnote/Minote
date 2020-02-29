/**
 * SMAA neighborhood blending vertex shader
 * @file
 * Adapted from https://github.com/turol/smaaDemo
 */

#version 330 core

out vec2 fTexCoords;
out vec4 fOffset;

uniform vec4 screenSize;

#define SMAA_RT_METRICS screenSize
#define SMAA_GLSL_3 1

#define SMAA_INCLUDE_VS 1
#define SMAA_INCLUDE_PS 0

#define SMAA_PRESET_ULTRA
#include "../../lib/smaa/smaa.h"
#include "util.h"

void main()
{
    vec2 pos = triangleVertex(gl_VertexID, fTexCoords);
    fTexCoords = flipTexCoord(fTexCoords);

    fOffset = vec4(0.0, 0.0, 0.0, 0.0);
    SMAANeighborhoodBlendingVS(fTexCoords, fOffset);
    gl_Position = vec4(pos, 1.0, 1.0);
}

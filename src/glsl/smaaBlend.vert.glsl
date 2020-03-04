/**
 * SMAA blending weight calculation vertex shader
 * @file
 * Adapted from https://github.com/turol/smaaDemo
 */

#version 330 core

out vec2 fTexCoords;
out vec2 fPixCoords;
out vec4 fOffset[3];

uniform vec4 screenSize;

#define SMAA_RT_METRICS screenSize
#define SMAA_INCLUDE_VS 1
#define SMAA_INCLUDE_PS 0
#include "smaaParams.glslh"
#include "util.glslh"

void main()
{
    vec2 pos = triangleVertex(gl_VertexID, fTexCoords);
    fTexCoords = flipTexCoord(fTexCoords);

    fOffset[0] = vec4(0.0, 0.0, 0.0, 0.0);
    fOffset[1] = vec4(0.0, 0.0, 0.0, 0.0);
    fOffset[2] = vec4(0.0, 0.0, 0.0, 0.0);
    fPixCoords = vec2(0.0, 0.0);
    SMAABlendingWeightCalculationVS(fTexCoords, fPixCoords, fOffset);
    gl_Position = vec4(pos, 1.0, 1.0);
}

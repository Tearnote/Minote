// Minote - glsl/smaaEdge.frag.glsl
// SMAA edge detection stage.
// Adapted from https://github.com/turol/smaaDemo

#version 330 core

in vec2 fTexCoords;
in vec4 fOffset[3];

out vec4 outColor;

uniform sampler2D image;
uniform vec4 screenSize;

#define SMAA_RT_METRICS screenSize
#define SMAA_INCLUDE_VS 0
#define SMAA_INCLUDE_PS 1
#include "smaaParams.glslh"

void main()
{
    outColor = vec4(SMAAColorEdgeDetectionPS(fTexCoords, fOffset, image), 0.0, 0.0);
}

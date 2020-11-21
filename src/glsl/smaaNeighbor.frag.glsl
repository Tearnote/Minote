// Minote - glsl/smaaNeighbor.frag.glsl
// SMAA neighborhood blending stage.
// Adapted from https://github.com/turol/smaaDemo

#version 330 core

in vec2 fTexCoords;
in vec4 fOffset;

out vec4 outColor;

uniform sampler2D image;
uniform sampler2D blend;
uniform float alpha;
uniform vec4 screenSize;

#define SMAA_RT_METRICS screenSize
#define SMAA_INCLUDE_VS 0
#define SMAA_INCLUDE_PS 1
#include "smaaParams.glslh"
#include "util.glslh"

void main()
{
    vec4 color = SMAANeighborhoodBlendingPS(fTexCoords, fOffset, image, blend);
    outColor = vec4(color.rgb, color.a * alpha);
}

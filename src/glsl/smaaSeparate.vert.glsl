/**
 * SMAA multisample separation vertex shader
 * @file
 * Adapted from https://github.com/turol/smaaDemo
 */

#version 330 core

out vec2 fTexCoords;

#include "util.glslh"

void main()
{
    vec2 pos = triangleVertex(gl_VertexID, fTexCoords);
    fTexCoords = flipTexCoord(fTexCoords);
    gl_Position = vec4(pos, 1.0, 1.0);
}

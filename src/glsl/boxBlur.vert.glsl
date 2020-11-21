// Minote - glsl/boxBlur.vert.glsl
// Blurs an input image by sampling 4 points diagonally. Generates its own
// vertices - just draw 3 triangles with no buffers attached.

#version 330 core

out vec2 fTexCoords;

#include "util.glslh"

void main()
{
    vec2 pos = triangleVertex(gl_VertexID, fTexCoords);
    gl_Position = vec4(pos, 0, 1);
}

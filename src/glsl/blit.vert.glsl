// Minote - glsl/blit.vert.glsl
// Basic fullscreen blit. Generates its own vertices, just draw 3 vertices
// with no buffers attached.

#version 330 core

out vec2 fTexCoords;

#include "util.glslh"

void main()
{
    vec2 pos = triangleVertex(gl_VertexID, fTexCoords);
    gl_Position = vec4(pos, 0, 1);
}

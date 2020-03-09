/**
 * Box blur vertex shader
 * @file
 * Basic fullscreen blit function. Generates its own vertices, just draw
 * 3 vertices with no buffers attached.
 */

#version 330 core

out vec2 fTexCoords;

void main()
{
    float x = -1.0 + float((gl_VertexID & 1) << 2);
    float y = -1.0 + float((gl_VertexID & 2) << 1);
    fTexCoords.x = (x+1.0)*0.5;
    fTexCoords.y = (y+1.0)*0.5;
    gl_Position = vec4(x, y, 0, 1);
}

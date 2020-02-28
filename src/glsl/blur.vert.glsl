/**
 * Blur vertex shader
 * @file
 * A box blur pass. Execute multiple times with an output texture 2x smaller
 * than the input in both dimensions.
 */

#version 330 core

out vec2 fTexCoords;

// Hardcoded fullscreen triangle. Call with 3 vertices.
void main()
{
    float x = -1.0 + float((gl_VertexID & 1) << 2);
    float y = -1.0 + float((gl_VertexID & 2) << 1);
    fTexCoords.x = (x+1.0)*0.5;
    fTexCoords.y = (y+1.0)*0.5;
    gl_Position = vec4(x, y, 0, 1);
}

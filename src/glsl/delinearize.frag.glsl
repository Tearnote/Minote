// Minote - glsl/delinearize.frag.glsl
// Blits a texture with gamma correction. Generates its own vertices, just draw
// 3 vertices with no buffers attached.

#version 330 core

in vec2 fTexCoords;

out vec4 outColor;

uniform sampler2D image;

#include "util.glslh"

void main()
{
    vec4 color = texture(image, fTexCoords);
    outColor = vec4(srgbEncode(color.rgb), color.a);
}

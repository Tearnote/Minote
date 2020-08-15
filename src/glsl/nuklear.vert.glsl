/**
 * Nuklear vertex shader
 * @file
 * Renders the output of Nuklear's vertex buffer generator.
 */

#version 330

layout(location = 0) in vec2 vPosition;
layout(location = 1) in vec2 vTexCoord;
layout(location = 2) in vec4 vColor;

out vec2 fTexCoord;
out vec4 fColor;

uniform mat4 projection;

#include "util.glslh"

void main()
{
    fTexCoord = vTexCoord;
    fColor = vec4(srgbDecode(vColor.rgb), vColor.a);
    gl_Position = projection * vec4(vPosition.xy, 0.0, 1.0);
}

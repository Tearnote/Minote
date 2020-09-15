/**
 * MSDF vertex shader
 * @file
 * Multi-channel signed distance field drawing, used for sharp text at any
 * zoom level.
 */

#version 330 core

layout(location = 0) in vec2 iPosition;
layout(location = 1) in vec2 iSize;
layout(location = 2) in vec4 iTexBounds;
layout(location = 3) in vec4 iColor;
layout(location = 4) in mat4 iTransform;

out vec4 fColor;
out vec2 fTexCoords;

uniform mat4 camera;
uniform mat4 projection;

void main()
{
    vec2 vertex = vec2(gl_VertexID % 2, gl_VertexID / 2);
    vec4 position = vec4(vertex * iSize + iPosition, 0.0, 1.0);
    gl_Position = projection * camera * iTransform * position;
    fColor = iColor;
    fTexCoords = mix(iTexBounds.xy, iTexBounds.zw, vertex);
}

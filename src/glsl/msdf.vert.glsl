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
layout(location = 4) in int iTransformIndex;

out vec4 fColor;
out vec2 fTexCoords;

uniform mat4 camera;
uniform mat4 projection;
uniform samplerBuffer transforms;

void main()
{
    mat4 transform;
    transform[0] = texelFetch(transforms, iTransformIndex * 4 + 0);
    transform[1] = texelFetch(transforms, iTransformIndex * 4 + 1);
    transform[2] = texelFetch(transforms, iTransformIndex * 4 + 2);
    transform[3] = texelFetch(transforms, iTransformIndex * 4 + 3);

    vec2 vertex = vec2(gl_VertexID % 2, gl_VertexID / 2);
    vec2 stringVertex = vertex * iSize + iPosition;
    vec4 position = vec4(0.0, stringVertex.y, stringVertex.x, 1.0);

    gl_Position = projection * camera * transform * position;
    fColor = iColor;
    fTexCoords = mix(iTexBounds.xy, iTexBounds.zw, vertex);
}

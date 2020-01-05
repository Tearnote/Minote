/**
 * Flat shading vertex shader
 * @file
 * No lighting, just constant color + per-instance tint.
 */

#version 330 core

layout(location = 0) in vec3 vPosition; ///< xyz coordinate of the vertex
layout(location = 1) in vec4 vColor; ///< rgba color of the vertex
layout(location = 2) in vec4 iTint; ///< rgba tint of the instance
layout(location = 3) in mat4 iModel; ///< transformation matrix of the instance

out vec4 fColor; ///< rgba output color

uniform mat4 camera; ///< view transform matrix
uniform mat4 projection; ///< projection matrix

/// shader entry point
void main()
{
    gl_Position = projection * camera * iModel * vec4(vPosition, 1.0);
    fColor = vColor * iTint;
}

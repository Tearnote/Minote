/**
 * Phong shading vertex shader
 * @file
 * Basic Phong lighting model with one light source and per-instance tint.
 */

#version 330 core

layout(location = 0) in vec3 vPosition; ///< coordinate of the vertex
layout(location = 1) in vec4 vColor; ///< color of the vertex
layout(location = 2) in vec3 vNormal; ///< normal vector of the vertex
layout(location = 3) in vec4 iTint; ///< tint of the instance
layout(location = 4) in mat4 iModel; ///< transformation matrix of the instance

out vec3 fPosition; ///< fragment position in view space
out vec4 fColor; ///< fragment color
out vec3 fNormal; ///< fragment normal vector in view space
out vec3 fLightPosition; ///< position of the light source in view space

uniform mat4 camera; ///< view transform matrix
uniform mat4 projection; ///< projection matrix
uniform vec3 lightPosition; ///< position of the light source in world space

/// shader entry point
void main()
{
    gl_Position = projection * camera * iModel * vec4(vPosition, 1.0);
    fPosition = vec3(camera * iModel * vec4(vPosition, 1.0));
    fNormal = mat3(transpose(inverse(camera * iModel))) * vNormal;
    fColor = vColor * iTint;
    fLightPosition = vec3(camera * vec4(lightPosition, 1.0));
}

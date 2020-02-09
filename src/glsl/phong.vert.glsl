/**
 * Phong shading vertex shader
 * @file
 * Basic Phong lighting model with one light source and per-instance tint.
 */

#version 330 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec4 vColor;
layout(location = 2) in vec3 vNormal;
layout(location = 3) in vec4 iTint;
layout(location = 4) in mat4 iModel;

out vec3 fPosition; // in view space
out vec4 fColor;
out vec3 fNormal; // in view space
out vec3 fLightPosition; // in view space

uniform mat4 camera;
uniform mat4 projection;
uniform vec3 lightPosition; // in world space

void main()
{
    gl_Position = projection * camera * iModel * vec4(vPosition, 1.0);
    fPosition = vec3(camera * iModel * vec4(vPosition, 1.0));
    fNormal = mat3(transpose(inverse(camera * iModel))) * vNormal;
    fColor = vColor * iTint;
    fLightPosition = vec3(camera * vec4(lightPosition, 1.0));
}

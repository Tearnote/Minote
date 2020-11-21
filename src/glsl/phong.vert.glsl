// Minote - glsl/phong.vert.glsl
// Basic Phong-Blinn lighting model with one light source and per-instance tint.
// Fragment stage inputs are transformed to view space.

#version 330 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec4 vColor;
layout(location = 2) in vec3 vNormal;
layout(location = 3) in vec4 iTint;
layout(location = 4) in vec4 iHighlight;
layout(location = 5) in mat4 iModel;

out vec3 fPosition;
out vec4 fColor;
out vec4 fHighlight;
out vec3 fNormal;
out vec3 fLightPosition;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 lightPosition;

void main()
{
    gl_Position = projection * view * iModel * vec4(vPosition, 1.0);
    fPosition = vec3(view * iModel * vec4(vPosition, 1.0));
    fNormal = mat3(transpose(inverse(view * iModel))) * vNormal;
    fColor = vColor * iTint;
    fHighlight = iHighlight;
    fLightPosition = vec3(view * vec4(lightPosition, 1.0));
}

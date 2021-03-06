// Minote - glsl/flat.vert.glsl
// Draw vertices with no lighting, just constant color + per-instance tint.

#version 330 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec4 vColor;
layout(location = 2) in vec4 iTint;
layout(location = 3) in vec4 iHighlight;
layout(location = 4) in mat4 iModel;

out vec4 fColor;
out vec4 fHighlight;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * iModel * vec4(vPosition, 1.0);
    fColor = vColor * iTint;
    fHighlight = iHighlight;
}

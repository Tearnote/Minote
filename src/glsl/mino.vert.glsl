#version 330 core

layout (location = 0) in vec2 vPosition;
layout (location = 1) in vec2 vOffset;
layout (location = 2) in vec4 vColor;

out vec4 fColor;

uniform mat4 projection;

void main()
{
	gl_Position = projection * vec4(vPosition.xy + vOffset.xy, 0.0, 1.0);
	fColor = vColor;
}

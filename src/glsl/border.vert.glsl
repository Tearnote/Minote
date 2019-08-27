#version 330 core

layout(location = 0) in vec2 vPosition;
layout(location = 1) in vec2 vOffset1;
layout(location = 2) in vec2 vOffset2;

uniform mat4 camera;
uniform mat4 projection;

void main()
{
	vec4 worldPosition = vec4(vPosition, 0.0, 1.0);
	worldPosition.x *= vOffset2.x - vOffset1.x;
	worldPosition.y *= vOffset2.y - vOffset1.y;
	worldPosition.xy += vOffset1;
	gl_Position = projection * camera * worldPosition;
}

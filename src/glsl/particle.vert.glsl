#version 330 core

layout(location = 0) in vec2 vPosition;
layout(location = 1) in vec2 vOffset;
layout(location = 2) in vec2 vSize;
layout(location = 3) in float vDirection;
layout(location = 4) in vec4 vColor;

out vec4 fColor;

uniform mat4 camera;
uniform mat4 projection;

void main()
{
	float direction = vDirection + radians(180.0);
	vec4 worldPosition = vec4(vPosition, 0.0, 1.0);
	worldPosition.xy *= vSize;
	worldPosition.xy *= mat2(cos(direction), -sin(direction), sin(direction), cos(direction));
	worldPosition.xy += vOffset;
	gl_Position = projection * camera * worldPosition;
	fColor = vColor;
}
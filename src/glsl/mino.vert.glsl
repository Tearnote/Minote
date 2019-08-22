#version 330 core

layout(location = 0) in vec3 vPosition;

out vec4 fColor;

uniform mat4 model;
uniform mat4 camera;
uniform mat4 projection;

void main()
{
	gl_Position = projection * camera * model * vec4(vPosition, 1.0f);
	fColor = vec4(1.0f, pow(0.5f, 2.2f), pow(0.2f, 2.2f), 1.0f);
}

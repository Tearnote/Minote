#version 330 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;

out vec4 fColor;
out vec3 fPosition;
out vec3 fNormal;

uniform mat4 model;
uniform mat4 camera;
uniform mat4 projection;

void main()
{
	gl_Position = projection * camera * model * vec4(vPosition, 1.0);
	fColor = vec4(1.0, 0.22, 0.03, 1.0);
	fNormal = mat3(transpose(inverse(camera * model))) * vNormal;
	fPosition = vec3(camera * model * vec4(vPosition, 1.0));
}

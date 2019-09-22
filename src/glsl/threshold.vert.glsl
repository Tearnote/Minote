#version 330 core
layout (location = 0) in vec2 vPosition;
layout (location = 1) in vec2 vTexCoords;

out vec2 fTexCoords;

void main()
{
	gl_Position = vec4(vPosition.x, vPosition.y, 0.0, 1.0);
	fTexCoords = vTexCoords;
}
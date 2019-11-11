// Minote - glsl/text.vert.glsl

#version 330 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec2 vTexCoords;

out vec2 fTexCoords;

uniform mat4 camera;
uniform mat4 projection;

void main()
{
	gl_Position = projection * camera * vec4(vPosition, 1.0);
	fTexCoords = vTexCoords;
}
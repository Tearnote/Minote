#version 330 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec2 vOffset;
layout(location = 2) in vec2 vSize;
layout(location = 3) in vec2 vTexOffset;
layout(location = 4) in vec2 vTexSize;

out vec2 fTexCoord;

uniform mat4 camera;
uniform mat4 projection;

void main()
{
	vec4 newPosition = vec4(mix(vOffset, vOffset + vSize, vPosition.xy), 1.0, 1.0);
	gl_Position = projection * camera * newPosition;
	fTexCoord = mix(vTexOffset, vTexOffset + vTexSize, vPosition.xy);
	fTexCoord.y = 1.0 - fTexCoord.y;
}

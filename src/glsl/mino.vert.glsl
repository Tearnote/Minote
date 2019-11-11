// Minote - glsl/mino.vert.glsl

#version 330 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vOffset;
layout(location = 3) in vec4 vColor;
layout(location = 4) in float vHighlight;

out vec4 fColor;
out vec3 fPosition;
out vec3 fNormal;
out float fHighlight;
out float fVertical;

uniform mat4 camera;
uniform mat4 normalCamera;
uniform mat4 projection;

void main()
{
	vec4 worldPosition = vec4(vPosition, 1.0);
	worldPosition.xy += vOffset;
	gl_Position = projection * camera * worldPosition;
	fColor = vColor;
	fNormal = normalize(mat3(normalCamera) * vNormal);
	fPosition = vec3(camera * worldPosition);
	fHighlight = vHighlight;
	fVertical = vPosition.y;
}
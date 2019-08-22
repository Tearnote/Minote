#version 330 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vOffset;
layout(location = 3) in vec4 vColor;

out vec4 fColor;
out vec3 fPosition;
out vec3 fNormal;

uniform mat4 model;
uniform mat4 camera;
uniform mat4 projection;

void main()
{
	vec4 worldPosition = model * vec4(vPosition, 1.0);
	worldPosition.xy += vOffset;
	gl_Position = projection * camera * worldPosition;
	fColor = vColor;
	fNormal = mat3(transpose(inverse(camera * model))) * vNormal;
	fPosition = vec3(camera * worldPosition);
}

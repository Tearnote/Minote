#version 330 core

in vec4 fColor;
in vec3 fPosition;
in vec3 fNormal;

out vec4 outColor;

void main()
{
	vec3 normal = normalize(fNormal);
	vec3 lightPosition = vec3(2.0, 4.0, 8.0);
	vec3 lightColor = vec3(1.0, 1.0, 1.0);
	vec3 lightDirection = normalize(lightPosition - fPosition);

	float ambientStrength = pow(0.1, 2.2);
	vec3 ambient = ambientStrength * lightColor;
	float diffuseStrength = max(dot(normal, lightDirection), 0.0);
	vec3 diffuse = diffuseStrength * lightColor;

	outColor = vec4(ambient + diffuse, 1.0) * fColor;
}

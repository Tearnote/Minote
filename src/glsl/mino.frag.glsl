#version 330 core

in vec4 fColor;
in vec3 fPosition;
in vec3 fNormal;

out vec4 outColor;

void main()
{
	vec3 normal = normalize(fNormal);
	vec3 lightPosition = vec3(2.0, 5.0, 0.0);
	vec3 lightColor = vec3(1.0, 1.0, 1.0);
	vec3 lightDirection = normalize(fPosition - lightPosition);

	float ambientStrength = pow(0.1, 2.2);
	vec3 ambient = ambientStrength * lightColor;

	float diffuseStrength = max(dot(normal, -lightDirection), 0.0);
	vec3 diffuse = diffuseStrength * lightColor;

	float specularStrength = pow(0.5, 2.2);
	vec3 viewDirection = normalize(-fPosition);
	vec3 reflectDirection = reflect(lightDirection, normal);
	float spec = pow(max(dot(viewDirection, reflectDirection), 0.0), 32);
	vec3 specular = specularStrength * spec * lightColor;

	outColor = vec4(ambient + diffuse + specular, 1.0) * fColor;
}

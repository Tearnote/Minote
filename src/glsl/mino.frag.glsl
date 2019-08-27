#version 330 core

in vec4 fColor;
in vec3 fPosition;
in vec3 fNormal;
in float fVertical;

out vec4 outColor;

uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform float ambientStrength;
uniform float diffuseStrength;
uniform float specularStrength;
uniform float shininess;

void main()
{
	vec3 lightDirection = normalize(fPosition - lightPosition);
	vec3 viewDirection = normalize(-fPosition);
	vec3 reflectDirection = reflect(lightDirection, fNormal);

	vec3 ambient = ambientStrength * lightColor;
	vec3 diffuse = diffuseStrength * max(dot(fNormal, -lightDirection), 0.0) * lightColor;
	float shine = pow(max(dot(viewDirection, reflectDirection), 0.0), shininess);
	vec3 specular = specularStrength * shine * lightColor;

	float colorMod = mix(0.5, 1.5, fVertical);
	vec4 newColor = vec4(vec3(fColor) * colorMod, fColor.a);
	outColor = vec4(ambient + diffuse + specular, 1.0) * newColor;
}

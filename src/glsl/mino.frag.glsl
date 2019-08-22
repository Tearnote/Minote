#version 330 core
in vec4 fColor;

out vec4 outColor;

void main()
{
	vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
	float ambientStrength = pow(0.1f, 2.2f);

	vec3 ambient = ambientStrength * lightColor;
	outColor = fColor * vec4(ambient, 1.0f);
}

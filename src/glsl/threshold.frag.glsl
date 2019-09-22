#version 330 core

in vec2 fTexCoords;

out vec4 outColor;

uniform sampler2D screen;

void main()
{
	vec4 fragColor = texture(screen, fTexCoords);
	float brightness = dot(fragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
	if(brightness > 0.8)
		outColor = vec4(fragColor.rgb, 1.0);
	else
		outColor = vec4(0.0, 0.0, 0.0, 1.0);
}
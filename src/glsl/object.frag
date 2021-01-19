#version 460

layout(location = 0) in flat uint material;
layout(location = 1) in flat uint drawID;
layout(location = 2) in flat uint instanceID;
layout(location = 3) in vec4 f_position;
layout(location = 4) in vec4 f_color;
layout(location = 5) in vec4 f_normal;
layout(location = 6) in vec4 f_lightPosition; // in view space

layout(location = 0) out vec4 out_color;

#include "world.glslh"
#include "object.glslh"

void main() {
	const vec4 i_highlight = instances.data[instanceID].highlight;

	switch (material) {

	case MATERIAL_FLAT:

		vec4 color = f_color;
		out_color = vec4(mix(color.rgb, i_highlight.rgb, i_highlight.a), color.a);
		break;

	case MATERIAL_PHONG:
		const float ambient = phongCommands.data[drawID].ambient;
		const float diffuse = phongCommands.data[drawID].diffuse;
		const float specular = phongCommands.data[drawID].specular;
		const float shine = phongCommands.data[drawID].shine;
//		const float ambient = 0.2;
//		const float diffuse = 0.9;
//		const float specular = 0.4;
//		const float shine = 24.0;

		vec4 normal = normalize(f_normal);
		vec4 lightDirection = normalize(f_lightPosition - f_position);
		vec4 viewDirection = normalize(-f_position);
		vec4 halfwayDirection = normalize(lightDirection + viewDirection);

		vec4 outAmbient = world.ambientColor * ambient;
		vec4 outDiffuse = world.lightColor * diffuse * max(dot(normal, lightDirection), 0.0);
		vec4 outSpecular = world.lightColor * specular * pow(max(dot(normal, halfwayDirection), 0.0), shine);
		vec4 lightStrength = outAmbient + outDiffuse + outSpecular;

		vec4 litColor = vec4(lightStrength.rgb * f_color.rgb, f_color.a);
		out_color = vec4(mix(litColor.rgb, i_highlight.rgb, i_highlight.a), litColor.a);
		break;

	}
}

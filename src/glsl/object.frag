#version 460
#pragma shader_stage(fragment)

layout(location = 0) in flat uint InstanceIndex;
layout(location = 1) in vec4 f_position;
layout(location = 2) in vec4 f_color;
layout(location = 3) in vec4 f_normal;
layout(location = 4) in vec4 f_lightPosition; // in view space
layout(location = 5) in vec4 f_lightSpacePosition;

layout(location = 0) out vec4 out_color;

layout(binding = 2) uniform sampler2D shadowmap;

#include "object.glslh"

void main() {
	const Instance instance = instances.data[InstanceIndex];

	vec4 normal = normalize(f_normal);
	vec4 lightDirection = normalize(f_lightPosition - f_position);
	vec4 viewDirection = normalize(-f_position);
	vec4 halfwayDirection = normalize(lightDirection + viewDirection);

	vec3 lightCoord = f_lightSpacePosition.xyz / f_lightSpacePosition.w;
	lightCoord.rg = lightCoord.rg * 0.5 + 0.5;
	float closestDepth = texture(shadowmap, lightCoord.xy).r;
	float currentDepth = lightCoord.z;
	float bias = max(0.016 * (1.0 - dot(normal, lightDirection)), 0.001);
	float shadow = currentDepth - bias > closestDepth ? 0.0 : 1.0;

	vec4 outAmbient = world.ambientColor * instance.ambient;
	vec4 outDiffuse = world.lightColor * instance.diffuse * max(dot(normal, lightDirection), 0.0);
	vec4 outSpecular = world.lightColor * instance.specular * pow(max(dot(normal, halfwayDirection), 0.0), instance.shine);
	vec4 lightStrength = outAmbient + shadow * (outDiffuse + outSpecular);

	vec4 litColor = vec4(lightStrength.rgb * f_color.rgb, f_color.a);
	out_color = vec4(mix(litColor.rgb, instance.highlight.rgb, instance.highlight.a), litColor.a);
}

#version 460
#pragma shader_stage(fragment)

layout(location = 0) in flat uint InstanceIndex;
layout(location = 1) in vec3 f_position;
layout(location = 2) in vec4 f_color;
layout(location = 3) in vec3 f_normal;
layout(location = 4) in vec3 f_viewPosition;

layout(location = 0) out vec4 out_color;

layout(binding = 3) uniform samplerCube cubemap;

#include "object.glslh"

vec3 envBRDFApprox(vec3 f0, float NoV, float roughness) {
	vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
	vec4 c1 = vec4(1.0, 0.0425, 1.04, -0.04);
	vec4 r = roughness * c0 + c1;
	float a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
	vec2 AB = vec2(-1.04, 1.04) * a004 + r.zw;
	return f0 * AB.x + AB.y;
}

void main() {
	const Instance instance = instances.data[InstanceIndex];

	const float mipCount = floor(log2(float(textureSize(cubemap, 0).x))) + 1.0;

	// Standard vectors
	vec3 normal = normalize(f_normal);
	vec3 viewDirection = normalize(f_viewPosition - f_position);
	float NoV = dot(normal, viewDirection);

	// PBR calculation
	vec3 f0 = max(f_color.rgb * instance.metalness, vec3(0.04));

	vec3 diffuse = f_color.rgb * textureLod(cubemap, normal, mipCount - 1.0).rgb * (1.0 - instance.metalness);
	vec3 specular = vec3(textureLod(cubemap, -reflect(viewDirection, normal), mipCount - (1 - 1.2 * log2(instance.roughness))));

	out_color = vec4(mix(diffuse, specular, envBRDFApprox(f0, NoV, instance.roughness)), f_color.a);
//	out_color = vec4(envBRDFApprox(f0, NoV, instance.roughness), f_color.a);
}

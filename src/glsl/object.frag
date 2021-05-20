#version 460
#pragma shader_stage(fragment)

layout(location = 0) in flat uint InstanceIndex;
layout(location = 1) in vec3 f_position;
layout(location = 2) in vec4 f_color;
layout(location = 3) in vec3 f_normal;
layout(location = 4) in vec3 f_viewPosition;

layout(location = 0) out vec4 out_color;

layout(binding = 3) uniform samplerCube cubemap;
layout(binding = 4) uniform sampler3D aerialPerspective;

layout(push_constant) uniform Constants {
	vec3 sunDirection;
};

#include "object.glsl"
#include "util.glsl"

vec3 envBRDFApprox(vec3 f0, float NoV, float roughness) {
	vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
	vec4 c1 = vec4(1.0, 0.0425, 1.04, -0.04);
	vec4 r = roughness * c0 + c1;
	float a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
	vec2 AB = vec2(-1.04, 1.04) * a004 + r.zw;
	return f0 * AB.x + AB.y;
}

float D_Approx(float Roughness, float RoL) {
    float a = Roughness * Roughness;
    float a2 = a * a;
    float rcp_a2 = 1.0 / a2;
    // 0.5 / ln(2), 0.275 / ln(2)
    float c = 0.72134752 * rcp_a2 + 0.39674113;
    return rcp_a2 * exp2(c*RoL - c);
}

#define AP_KM_PER_SLICE 4.0
float AerialPerspectiveDepthToSlice(float depth) {
	return depth * (1.0 / AP_KM_PER_SLICE);
}

void main() {
	const Instance instance = instances.data[InstanceIndex];

	const float mipCount = 8;

	// Standard vectors
	vec3 normal = normalize(f_normal);
	vec3 viewDirection = normalize(f_viewPosition - f_position);
	float NoV = dot(normal, viewDirection);

	// Sun visibility
	float sunDot = dot(vec3(0.0, 0.0, 1.0), sunDirection);
	vec3 sunColor = vec3(textureLod(cubemap, sunDirection, 0.0));
	sunColor = mix(vec3(1.0), sunColor, Luminance(sunColor));
	const float sunAngularSize = radians(0.4);
	sunColor *= smoothstep(cos(radians(90) + sunAngularSize), cos(radians(90) - sunAngularSize), sunDot);

	// PBR calculation
	vec3 f0 = max(f_color.rgb * instance.metalness, vec3(0.04));

	vec3 iblDiffuse = textureLod(cubemap, normal, mipCount - 2.0).rgb;
	vec3 sunDiffuse = sunColor * max(dot(normal, sunDirection), 0.0);
	vec3 diffuse = f_color.rgb * (iblDiffuse + sunDiffuse) * (1.0 - instance.metalness);

	vec3 reflection = reflect(viewDirection, normal);
	float iblMip = max(7.0 - 0.480898 * log(2.0 / pow(instance.roughness, 4.0) - 1.0), 0.0);
	vec3 iblSpecular = vec3(textureLod(cubemap, -reflection, iblMip));
	const float sunMinRoughness = 1.0 / 16.0;
	vec3 sunSpecular = sunColor * D_Approx(max(instance.roughness, sunMinRoughness), dot(-reflection, sunDirection));
	vec3 specular = iblSpecular + sunSpecular;

	out_color = vec4(mix(diffuse, specular, envBRDFApprox(f0, NoV, instance.roughness)), f_color.a);

	// Aerial perspective
	float depth = 1.0 - gl_FragCoord.z; // Reverse-z
	float Slice = AerialPerspectiveDepthToSlice(depth);
	float Weight = 1.0;
	if (Slice < 0.5) {
		// We multiply by weight to fade to 0 at depth 0. That works for luminance and opacity.
		Weight = clamp(Slice * 2.0, 0.0, 1.0);
		Slice = 0.5;
	}
	float w = sqrt(Slice / textureSize(aerialPerspective, 0).z);	// squared distribution

	const vec4 AP = Weight * textureLod(aerialPerspective, vec3(gl_FragCoord.xy / vec2(world.viewportSize), w), 0.0);
	out_color.rgb += AP.rgb * 4.0;
}

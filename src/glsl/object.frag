#version 460
#pragma shader_stage(fragment)

#include "sky/skyAccess.glsl"
#include "types.glsl"
#include "util.glsl"

layout(location = 0) in flat uint InstanceIndex;
layout(location = 1) in vec3 f_position;
layout(location = 2) in vec4 f_color;
layout(location = 3) in vec3 f_normal;
layout(location = 4) in vec3 f_viewPosition;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform WorldConstants {
	World u_world;
};
layout(std430, binding = 1) restrict readonly buffer RowTransforms {
	RowTransform b_transforms[];
};
layout(std430, binding = 2) restrict readonly buffer Materials {
	Material b_materials[];
};
layout(binding = 3) restrict readonly buffer SunLuminance {
	vec3 b_sunLuminance;
};

layout(binding = 4) uniform samplerCube s_cubemap;
layout(binding = 5) uniform sampler3D s_aerialPerspective;

vec3 envBRDFApprox(vec3 _f0, float _NoV, float _roughness) {
	
	vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
	vec4 c1 = vec4(1.0, 0.0425, 1.04, -0.04);
	vec4 r = _roughness * c0 + c1;
	float a004 = min(r.x * r.x, exp2(-9.28 * _NoV)) * r.x + r.y;
	vec2 ab = vec2(-1.04, 1.04) * a004 + r.zw;
	return _f0 * ab.x + ab.y;
	
}

float dApprox(float _roughness, float _RoL) {
	
	float a = _roughness * _roughness;
	float a2 = a * a;
	float rcp_a2 = 1.0 / a2;
	// 0.5 / ln(2), 0.275 / ln(2)
	float c = 0.72134752 * rcp_a2 + 0.39674113;
	return rcp_a2 * exp2(c*_RoL - c);
	
}

const float IBLMipCount = 8.0;

void main() {
	
	const Material material = b_materials[InstanceIndex];
	
	// Standard vectors
	
	vec3 normal = normalize(f_normal);
	vec3 viewDirection = normalize(f_viewPosition - f_position);
	float NoV = dot(normal, viewDirection);
	
	// Empirical sun visibility above the horizon
	
	float sunDot = dot(vec3(0.0, 0.0, 1.0), u_world.sunDirection);
	vec3 sunColor = b_sunLuminance;
	
	const float sunAngularSize = radians(0.2);
	const float sunsetStart = cos(radians(90.05) - sunAngularSize);
	const float sunsetEnd = cos(radians(90.05) + sunAngularSize);
	float sunset = (sunDot - sunsetEnd) / (sunsetStart - sunsetEnd);
	sunset = clamp(sunset, 0.0, 1.0);
	sunset = (1.0 - cos(sunset * 1.57079633)) / 2.0;
	
	sunColor *= sunset;
	
	// PBR calculation
	
	vec3 f0 = max(f_color.rgb * material.metalness, vec3(0.04));
	
	vec3 iblDiffuse = textureLod(s_cubemap, normal, IBLMipCount - 2.0).rgb;
	vec3 sunDiffuse = sunColor * max(dot(normal, u_world.sunDirection), 0.0);
	vec3 diffuse = f_color.rgb * (iblDiffuse + sunDiffuse) * (1.0 - material.metalness);
	
	vec3 reflection = reflect(viewDirection, normal);
	float iblMip = max(7.0 - 0.480898 * log(2.0 / pow(material.roughness, 4.0) - 1.0), 0.0);
	vec3 iblSpecular = vec3(textureLod(s_cubemap, -reflection, iblMip));
	const float SunMinRoughness = 1.0 / 16.0;
	vec3 sunSpecular = sunColor * dApprox(max(material.roughness, SunMinRoughness), dot(-reflection, u_world.sunDirection));
	vec3 specular = iblSpecular + sunSpecular;
	
	vec4 surface = vec4(mix(diffuse, specular, envBRDFApprox(f0, NoV, material.roughness)), f_color.a);
	
	// Aerial perspective
	
	float depth = 1.0 - gl_FragCoord.z; // Reverse-z
	float slice = aerialPerspectiveDepthToSlice(depth);
	float weight = 1.0;
	if (slice < 0.5) {
		
		// We multiply by weight to fade to 0 at depth 0. That works for luminance and opacity.
		weight = clamp(slice * 2.0, 0.0, 1.0);
		slice = 0.5;
		
	}
	float w = sqrt(slice / textureSize(s_aerialPerspective, 0).z);	// squared distribution
	
	vec4 ap = weight * textureLod(s_aerialPerspective, vec3(gl_FragCoord.xy / vec2(u_world.viewportSize), w), 0.0);
	out_color = surface + ap;
	
}

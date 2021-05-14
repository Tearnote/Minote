#version 460
#pragma shader_stage(fragment)

layout(location = 0) in vec2 f_texCoords;

layout(location = 0) out vec4 out_color;

layout(set = 1, binding = 0) uniform sampler2D skyView;

#include "sky.glslh"

void main() {
	const ivec2 viewSize = textureSize(skyView, 0);

	vec3 ClipSpace = vec3(f_texCoords * vec2(2.0, 2.0) - vec2(1.0, 1.0), 1.0);
	vec4 HPos = gSkyInvViewProjMat * vec4(ClipSpace, 1.0);

	vec3 WorldDir = normalize(HPos.xyz);
	vec3 WorldPos = camera + vec3(0.0, 0.0, Atmosphere.BottomRadius);

	float viewHeight = length(WorldPos);
	
	if (viewHeight < Atmosphere.TopRadius) {
		vec2 uv;
		vec3 UpVector = normalize(WorldPos);
		float viewZenithCosAngle = dot(WorldDir, UpVector);

		vec3 sideVector = normalize(cross(UpVector, WorldDir)); // assumes non parallel vectors
		vec3 forwardVector = normalize(cross(sideVector, UpVector)); // aligns toward the sun light but perpendicular to up vector
		vec2 lightOnPlane = vec2(dot(sun_direction, forwardVector), dot(sun_direction, sideVector));
		lightOnPlane = normalize(lightOnPlane);
		float lightViewCosAngle = lightOnPlane.x;

		bool IntersectGround = raySphereIntersectNearest(WorldPos, WorldDir, vec3(0.0), Atmosphere.BottomRadius) >= 0.0;

		SkyViewLutParamsToUv(IntersectGround, viewZenithCosAngle, lightViewCosAngle, viewSize, viewHeight, uv);
		out_color = vec4(textureLod(skyView, uv, 0.0).rgb + GetSunLuminance(WorldPos, WorldDir, Atmosphere.BottomRadius), 1.0);
		out_color.rgb *= 10.0;
	} else {
		out_color = vec4(0.0, 0.0, 0.0, 1.0);
	}
}

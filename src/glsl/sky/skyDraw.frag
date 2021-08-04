#version 460
#pragma shader_stage(fragment)

#include "../include/world.glsl"
#include "sky.glsl"

layout(location = 0) in vec2 f_texCoords;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform WorldConstants {
	World world;
};

layout(set = 1, binding = 0) uniform sampler2D skyView;

void main() {
	const ivec2 viewSize = textureSize(skyView, 0);

	vec3 ClipSpace = vec3(f_texCoords * vec2(2.0, 2.0) - vec2(1.0, 1.0), 0.0);
	vec4 HPos = world.viewProjectionInverse * vec4(ClipSpace, 1.0);

	vec3 WorldDir = normalize(HPos.xyz);
	vec3 WorldPos = world.cameraPos + vec3(0.0, 0.0, atmo.bottomRadius);

	float viewHeight = length(WorldPos);

	vec2 uv;
	vec3 UpVector = normalize(WorldPos);
	float viewZenithCosAngle = dot(WorldDir, UpVector);

	vec3 sideVector = normalize(cross(UpVector, WorldDir)); // assumes non parallel vectors
	vec3 forwardVector = normalize(cross(sideVector, UpVector)); // aligns toward the sun light but perpendicular to up vector
	vec2 lightOnPlane = vec2(dot(world.sunDirection, forwardVector), dot(world.sunDirection, sideVector));
	lightOnPlane = normalize(lightOnPlane);
	float lightViewCosAngle = lightOnPlane.x;

	bool IntersectGround = raySphereIntersectNearest(WorldPos, WorldDir, vec3(0.0), atmo.bottomRadius) >= 0.0;

	skyViewLutParamsToUv(IntersectGround, viewZenithCosAngle, lightViewCosAngle, viewSize, viewHeight, uv);
	vec3 skyView = textureLod(skyView, uv, 0.0).rgb;
	vec3 sun = getSunLuminance(WorldPos, WorldDir, world.sunDirection, world.sunIlluminance) * (120000.0 / world.sunIlluminance);
	out_color = vec4(skyView + sun, 1.0);
}

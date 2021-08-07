#version 460
#pragma shader_stage(fragment)

#include "../types.glsl"
#include "sky.glsl"

layout(location = 0) in vec2 f_texCoords;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform WorldConstants {
	World u_world;
};

layout(set = 1, binding = 0) uniform sampler2D s_skyView;

void main() {
	
	const ivec2 viewSize = textureSize(s_skyView, 0);
	
	vec3 clipSpace = vec3(f_texCoords * vec2(2.0) - vec2(1.0), 0.0);
	vec4 hPos = u_world.viewProjectionInverse * vec4(clipSpace, 1.0);

	vec3 worldDir = normalize(hPos.xyz);
	vec3 worldPos = u_world.cameraPos + vec3(0.0, 0.0, u_atmo.bottomRadius);

	float viewHeight = length(worldPos);

	vec2 uv;
	vec3 upVector = normalize(worldPos);
	float viewZenithCosAngle = dot(worldDir, upVector);

	vec3 sideVector = normalize(cross(upVector, worldDir)); // assumes non parallel vectors
	vec3 forwardVector = normalize(cross(sideVector, upVector)); // aligns toward the sun light but perpendicular to up vector
	vec2 lightOnPlane = vec2(dot(u_world.sunDirection, forwardVector), dot(u_world.sunDirection, sideVector));
	lightOnPlane = normalize(lightOnPlane);
	float lightViewCosAngle = lightOnPlane.x;

	bool intersectGround = (raySphereIntersectNearest(worldPos, worldDir, vec3(0.0), u_atmo.bottomRadius) >= 0.0);

	skyViewLutParamsToUv(intersectGround, viewZenithCosAngle, lightViewCosAngle, viewSize, viewHeight, uv, u_atmo.bottomRadius);
	vec3 skyView = textureLod(s_skyView, uv, 0.0).rgb;
	vec3 sun = getSunLuminance(worldPos, worldDir, u_world.sunDirection, u_world.sunIlluminance)
		* (120000.0 / u_world.sunIlluminance);
	out_color = vec4(skyView + sun, 1.0);
	
}

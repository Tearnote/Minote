#version 460
#pragma shader_stage(fragment)

layout(location = 0) in flat uint InstanceIndex;
layout(location = 1) in vec4 f_position;
layout(location = 2) in vec4 f_color;
layout(location = 3) in vec4 f_normal;
layout(location = 4) in vec4 f_lightPosition; // in view space
layout(location = 5) in vec4 f_lightSpacePosition;

layout(location = 0) out vec4 out_color;

layout(binding = 2) uniform sampler2D msmmoments;

#include "util.glslh"
#include "object.glslh"

// http://momentsingraphics.de/I3D2015.html
float ComputeMSMShadowIntensity(vec4 b, float FragmentDepth){
	float L32D22 = fma(-b[0], b[1], b[2]);
	float D22 = fma(-b[0], b[0], b[1]);
	float SquaredDepthVariance = fma(-b[1], b[1], b[3]);
	float D33D22 = dot(vec2(SquaredDepthVariance,-L32D22), vec2(D22, L32D22));
	float InvD22 = 1.0 / D22;
	float L32 = L32D22 * InvD22;
	vec3 z;
	z[0] = FragmentDepth;
	vec3 c = vec3(1.0, z[0], z[0] * z[0]);
	c[1] -= b.x;
	c[2] -= b.y + L32 * c[1];
	c[1] *= InvD22;
	c[2] *= D22 / D33D22;
	c[1] -= L32 * c[2];
	c[0] -= dot(c.yz, b.xy);
	float InvC2 = 1.0 / c[2];
	float p = c[1] * InvC2;
	float q = c[0] * InvC2;
	float r = sqrt((p * p  * 0.25) - q);
	z[1] = -p * 0.5 - r;
	z[2] = -p * 0.5 + r;
	vec4 Switch =
		(z[2]<z[0])? vec4(z[1], z[0], 1.0, 1.0) : (
		(z[1]<z[0])? vec4(z[0], z[1], 0.0, 1.0) :
	                 vec4(0.0,  0.0,  0.0, 0.0));
	float Quotient = (Switch[0] * z[2] - b[0] * (Switch[0] + z[2]) + b[1]) /
	                 ((z[2] - Switch[1]) * (z[0] - z[1]));
	return clamp(Switch[2] + Switch[3] * Quotient, 0.0, 1.0);
}

float expImpulse(float x, float k) {
	const float h = k*x;
	return h*exp(1.0-h);
}

void main() {
	const Instance instance = instances.data[InstanceIndex];

	// Direction vectors
	vec4 normal = normalize(f_normal);
	vec4 lightDirection = normalize(f_lightPosition - f_position);
	vec4 viewDirection = normalize(-f_position);
	vec4 halfwayDirection = normalize(lightDirection + viewDirection);

	// Shadow coverage
	const mat4 invBasis = mat4(
		-1.0/3.0, 0.0, -0.75, 0.0,
		0.0, 0.125, 0.0, -0.125,
		sqrt(3.0), 0.0, 0.75*sqrt(3.0), 0.0,
		0.0, 1.0, 0.0, 1.0
	);
	vec3 lightCoord = f_lightSpacePosition.xyz / f_lightSpacePosition.w;
	lightCoord.xy = lightCoord.xy * 0.5 + 0.5;
	vec4 moments = invBasis * (texture(msmmoments, lightCoord.xy) - vec4(0.5, 0.0, 0.5, 0.0));
	moments = mix(moments, vec4(0.0, 0.63, 0.0, 0.63), 6.0e-5);
	float shadow = ComputeMSMShadowIntensity(moments, lightCoord.z * 2.0 - 1.0);
	shadow = clamp(shadow / 0.98, 0.0, 1.0);
	shadow = mix(gain(shadow, 1.0 / fwidth(shadow)), shadow, smoothstep(0.16, 0.12, dot(normal, lightDirection)));
//	shadow = gain(shadow, 1.0 / fwidth(shadow));

	// Phong shading
	vec4 outAmbient = world.ambientColor * instance.ambient;
	vec4 outDiffuse = world.lightColor * instance.diffuse * max(dot(normal, lightDirection), 0.0);
	vec4 outSpecular = world.lightColor * instance.specular * pow(max(dot(normal, halfwayDirection), 0.0), instance.shine);
	vec4 lightStrength = outAmbient + (1.0 - shadow) * (outDiffuse + outSpecular);

	// Final blend
	out_color = vec4(lightStrength.rgb * f_color.rgb, f_color.a);
}

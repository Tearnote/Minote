layout(binding = 0) uniform World {

	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	mat4 viewProjectionInverse;
	uvec2 viewportSize;
	vec2 pad0;

	vec3 sunDirection;
	float multiScatteringFactor;

} world;
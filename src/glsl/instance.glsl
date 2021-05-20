struct Instance {
	mat4 transform;
	vec4 tint;
	float roughness;
	float metalness;
	uint meshID;
	float pad1;
};

layout(std430, binding = 1) readonly buffer Instances {
	Instance data[];
} instances;
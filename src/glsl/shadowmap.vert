#version 460
#pragma shader_stage(vertex)

layout(location = 0) in vec4 v_position;
layout(location = 1) in vec4 v_normal;
layout(location = 2) in vec4 v_color;

struct Instance {
	mat4 transform;
	vec4 tint;
	vec4 highlight;
	float ambient;
	float diffuse;
	float specular;
	float shine;
};

layout(std430, binding = 0) readonly buffer Instances {
	Instance data[];
} instances;

layout(binding = 1) uniform LightProjection {
	mat4 lightProjection;
};

void main() {
	const Instance instance = instances.data[gl_InstanceIndex];
	gl_Position = lightProjection * instance.transform * v_position;
}

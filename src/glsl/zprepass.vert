#version 460
#pragma shader_stage(vertex)

layout(location = 0) in vec3 v_position;

layout(std430, binding = 1) readonly buffer Transform {
	mat4 transform[];
};

#include "world.glsl"

void main() {
	gl_Position = world.viewProjection * transform[gl_InstanceIndex] * vec4(v_position, 1.0);
}

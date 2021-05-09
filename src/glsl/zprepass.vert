#version 460
#pragma shader_stage(vertex)

layout(location = 0) in vec3 v_position;

#include "object.glslh"

void main() {
	gl_Position = world.viewProjection * instances.data[gl_InstanceIndex].transform * vec4(v_position, 1.0);
}
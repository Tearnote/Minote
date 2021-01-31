#version 460

layout(location = 0) out flat uint material;
layout(location = 1) out flat uint drawID;
layout(location = 2) out flat uint instanceID;
layout(location = 3) out vec4 f_position;
layout(location = 4) out vec4 f_color;
layout(location = 5) out vec4 f_normal;
layout(location = 6) out vec4 f_lightPosition; // in view space

#include "world.glslh"
#include "object.glslh"

void main() {
	drawID = gl_DrawID;
	material = commands.data[drawID].material;
	instanceID = gl_InstanceIndex;

	const vec4 v_position = vertices.data[gl_VertexIndex].position;
	const vec4 v_normal = vertices.data[gl_VertexIndex].normal;
	const vec4 v_color = vertices.data[gl_VertexIndex].color;
	const mat4 i_transform = instances.data[gl_InstanceIndex].transform;
	const vec4 i_tint = instances.data[gl_InstanceIndex].tint;

	switch (material) {

	case MATERIAL_NONE:
		gl_Position = world.viewProjection * i_transform * v_position;
		break;

	case MATERIAL_FLAT:
		gl_Position = world.viewProjection * i_transform * v_position;
		f_color = v_color * i_tint;
		break;

	case MATERIAL_PHONG:
		gl_Position = world.viewProjection * i_transform * v_position;
		f_position = world.view * i_transform * v_position;
		f_color = v_color * i_tint;
		f_normal = vec4(mat3(transpose(inverse(world.view * i_transform))) * v_normal.xyz, 0.0);
		f_lightPosition = world.view * world.lightPosition;
		break;

	}
}

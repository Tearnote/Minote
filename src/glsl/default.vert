#version 460

layout(location = 0) out flat uint material;
layout(location = 1) out flat uint drawID;
layout(location = 2) out vec4 f_position;
layout(location = 3) out vec4 f_color;
layout(location = 4) out vec4 f_normal;
layout(location = 5) out vec4 f_lightPosition; // in view space

#include "world.glslh"

void main() {
	const vec4 v_position = vertices.data[gl_VertexIndex].position;
	const vec4 v_normal = vertices.data[gl_VertexIndex].normal;
	const vec4 v_color = vertices.data[gl_VertexIndex].color;
	const mat4 i_transform = instances.data[gl_InstanceIndex].transform;

	drawID = gl_DrawID;
	material = commands.data[drawID].material;

	switch (material) {

	case MATERIAL_FLAT:
		gl_Position = world.viewProjection * i_transform * v_position;
		f_color = v_color;
		break;

	case MATERIAL_PHONG:
		gl_Position = world.viewProjection * i_transform * v_position;
		f_position = world.view * i_transform * v_position;
		f_color = v_color;
		f_normal = vec4(mat3(transpose(inverse(world.view * i_transform))) * v_normal.xyz, 0.0);
		f_lightPosition = world.view * world.lightPosition;
		break;

	}
}

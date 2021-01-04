#version 460

layout(location = 0) in flat uint material;
layout(location = 1) in vec4 f_color;
layout(location = 2) in vec4 f_normal;

layout(location = 0) out vec4 out_color;

#include "world.glslh"

void main() {
	switch (material) {

	case MATERIAL_FLAT:
		out_color = f_color;
		break;

	case MATERIAL_PHONG:
		out_color = f_color;
		break;

	}
}

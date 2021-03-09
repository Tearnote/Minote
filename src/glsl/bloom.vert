#version 460

layout(location = 0) out vec2 f_texCoords;
layout(location = 1) out flat uint mode;

#include "util.glslh"

void main() {
	mode = gl_InstanceIndex;
	vec2 pos = triangleVertex(gl_VertexIndex, f_texCoords);
	gl_Position = vec4(pos, 0, 1);
}

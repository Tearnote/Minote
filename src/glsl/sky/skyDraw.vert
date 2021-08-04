#version 460
#pragma shader_stage(vertex)

layout(location = 0) out vec2 f_texCoords;

#include "../include/util.glsl"

void main() {
	vec2 pos = triangleVertex(gl_VertexIndex, f_texCoords);
	gl_Position = vec4(pos, 0.0, 1.0);
}

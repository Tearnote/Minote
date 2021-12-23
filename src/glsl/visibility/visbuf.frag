#version 460
#pragma shader_stage(fragment)

layout(location = 0) in flat uint InstanceIdx;

layout(location = 0) out uint out_visibility;

#include "visbuf.glsl"

void main() {
	
	out_visibility = (InstanceIdx << TRIANGLE_ID_BITS) + gl_PrimitiveID;
	
}

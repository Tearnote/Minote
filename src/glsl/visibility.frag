#version 460
#pragma shader_stage(fragment)

layout(location = 0) in flat uint InstanceIndex;

layout(location = 0) out uint out_visibility;

#include "visibility.glsl"

void main() {
	
	out_visibility = (InstanceIndex << TRIANGLE_ID_BITS) + gl_PrimitiveID;
	
}

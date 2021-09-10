#version 460
#pragma shader_stage(fragment)

layout(location = 0) in flat uint InstanceIndex;

layout(location = 0) out uint out_visibility;

#define TRIANGLE_ID_BITS 12

void main() {
	
	out_visibility = (InstanceIndex << TRIANGLE_ID_BITS) + gl_PrimitiveID;
	
}

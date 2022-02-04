#version 460
#pragma shader_stage(fragment)

layout(location = 0) out uint out_visibility;

void main() {
	
	out_visibility = gl_PrimitiveID;
	
}

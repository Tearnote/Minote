#version 460
#pragma shader_stage(fragment)

layout(location = 0) in vec2 f_texCoords;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D s_source;

vec3 uncharted2TonemapPartial(vec3 x) {
	
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
	
}

vec3 uncharted2Filmic(vec3 v) {
	
	float exposure_bias = 2.0;
	vec3 curr = uncharted2TonemapPartial(v * exposure_bias);
	
	vec3 W = vec3(8.0);
	vec3 white_scale = vec3(1.0) / uncharted2TonemapPartial(W);
	return curr * white_scale;
	
}

void main() {
	
	vec4 sourceTex = texture(s_source, f_texCoords);
	out_color = vec4(uncharted2Filmic(vec3(sourceTex)), 1.0);
	
}

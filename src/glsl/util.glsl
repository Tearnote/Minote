// Generic commonly used functions

#ifndef UTILS_GLSL
#define UTILS_GLSL

// Generate a bufferless screen-covering triangle from 3 vertices
// https://rauwendaal.net/2014/06/14/rendering-a-screen-covering-triangle-in-opengl/
vec2 triangleVertex(int _vertexID, out vec2 _texCoords) {
	
	float x = -1.0 + float((_vertexID & 1) << 2);
	float y = -1.0 + float((_vertexID & 2) << 1);
	_texCoords.x = (x+1.0)*0.5;
	_texCoords.y = (y+1.0)*0.5;
	return vec2(x, y);
	
}

// Generate a bufferless cube from 14 vertices. Draw with triangle strip
// https://twitter.com/donzanoid/status/616370134278606848
vec3 cubeVertex(int _vertexID) {
	
	int b = 1 << _vertexID;
	return vec3((0x287a & b) != 0, (0x02af & b) != 0, (0x31e3 & b) != 0);
	
}

// Conversion from linear RGB to sRGB
// http://www.java-gaming.org/topics/fast-srgb-conversion-glsl-snippet/37583/view.html
vec3 srgbEncode(vec3 _color) {
	
	float r = _color.r < 0.0031308 ? 12.92 * _color.r : 1.055 * pow(_color.r, 1.0/2.4) - 0.055;
	float g = _color.g < 0.0031308 ? 12.92 * _color.g : 1.055 * pow(_color.g, 1.0/2.4) - 0.055;
	float b = _color.b < 0.0031308 ? 12.92 * _color.b : 1.055 * pow(_color.b, 1.0/2.4) - 0.055;
	return vec3(r, g, b);
	
}

// Conversion from sRGB to linear RGB
vec3 srgbDecode(vec3 _color) {
	
	float r = _color.r < 0.04045 ? (1.0 / 12.92) * _color.r : pow((_color.r + 0.055) * (1.0 / 1.055), 2.4);
	float g = _color.g < 0.04045 ? (1.0 / 12.92) * _color.g : pow((_color.g + 0.055) * (1.0 / 1.055), 2.4);
	float b = _color.b < 0.04045 ? (1.0 / 12.92) * _color.b : pow((_color.b + 0.055) * (1.0 / 1.055), 2.4);
	return vec3(r, g, b);
	
}

float luminance(vec3 _color) {
	
	const vec3 W = vec3(0.2125, 0.7154, 0.0721);
	return dot(_color, W);
	
}

// https://github.com/hughsk/glsl-luma/blob/master/index.glsl
float luma(vec3 _color) {
	
	const vec3 W = vec3(0.299, 0.587, 0.114);
	return dot(_color, W);
	
}

// Retrieve u16 halves of a u32

uint u32Lower(uint _n) {
	
	return _n & ((1u << 16u) - 1u);
	
}

uint u32Upper(uint _n) {
	
	return _n >> 16u;
	
}

// https://graphics.pixar.com/library/OrthonormalB/paper.pdf
void orthonormalBasis(vec3 _n, out vec3 _b1, out vec3 _b2) {
	
	float sign = _n.z >= 0.0? 1.0 : -1.0;
	float a = -1.0 / (sign + _n.z);
	float b = _n.x * _n.y * a;
	_b1 = vec3(1.0 + sign * _n.x * _n.x * a, sign * b, -sign * _n.x);
	_b2 = vec3(b, sign + _n.y * _n.y * a, -_n.y);
	
}

#endif //UTILS_GLSL

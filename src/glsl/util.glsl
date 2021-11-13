// Generic commonly used functions

#ifndef UTILS_GLSL
#define UTILS_GLSL

// Useful primitives

float rcp(float _v) {
	
	return 1.0 / _v;
	
}

vec3 rcp(vec3 _v) {
	
	return vec3(1.0) / _v;
	
}

// https://github.com/CesiumGS/cesium/blob/main/Source/Shaders/Builtin/Functions/fastApproximateAtan.glsl
// Used under MIT license
float fastAtan(float _x) {
	
	return _x * (-0.1784 * _x - 0.0663 * _x * _x + 1.0301);
	
}

float fastAtan(float _y, float _x) {
    // atan approximations are usually only reliable over [-1, 1], or, in our case, [0, 1] due to modifications.
    // So range-reduce using abs and by flipping whether x or y is on top.
    float t = abs(_x); // t used as swap and atan result.
    float opposite = abs(_y);
    float adjacent = max(t, opposite);
    opposite = min(t, opposite);

    t = fastAtan(opposite / adjacent);

    // Undo range reduction
    t = abs(_y) > abs(_x)? 1.5707963267948966 - t : t;
    t = _x < 0.0? 3.141592653589793 - t : t;
    t = _y < 0.0? -t : t;
    return t;
}

float distanceSq(vec2 _a, vec2 _b) {
	
	vec2 d = _a - _b;
	return dot(d, d);
	
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
	
	vec3 W = vec3(0.2125, 0.7154, 0.0721);
	return dot(_color, W);
	
}

// https://github.com/hughsk/glsl-luma/blob/master/index.glsl
float luma(vec3 _color) {
	
	vec3 W = vec3(0.299, 0.587, 0.114);
	return dot(_color, W);
	
}

// Create a bitmask with n lowest bits set
uint bitmask(uint _n) {
	
	if (_n == 32u)
		return -1u;
	else
		return (1u << _n) - 1u;
	
}

// Retrieve u16 halves of a u32

uint u32Lower(uint _n) {
	
	return _n & bitmask(16);
	
}

uint u32Upper(uint _n) {
	
	return _n >> 16u;
	
}

// Create a u32 out of two u16s
uint u32Fromu16(uvec2 _val) {
	
	return (_val[1] << 16u) | _val[0];
	
}

// Create two u16s out of a u32
uvec2 u16Fromu32(uint _val) {
	
	return uvec2(u32Lower(_val), u32Upper(_val));
	
}

#define U16FROMU32(_v) \
	uvec2((_v) & 0xffffu, (_v) >> 16u)

// https://fgiesen.wordpress.com/2009/12/13/decoding-morton-codes/
uint mortonCompact1By1(uint _x) {
	
	_x &= 0x55555555u;                    // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
	_x = (_x ^ (_x >>  1)) & 0x33333333u; // x = --fe --dc --ba --98 --76 --54 --32 --10
	_x = (_x ^ (_x >>  2)) & 0x0f0f0f0fu; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
	_x = (_x ^ (_x >>  4)) & 0x00ff00ffu; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
	_x = (_x ^ (_x >>  8)) & 0x0000ffffu; // x = ---- ---- ---- ---- fedc ba98 7654 3210
	return _x;
	
}

uvec2 mortonOrder(uint _id) {
	
	return uvec2(mortonCompact1By1(_id), mortonCompact1By1(_id >> 1));
	
}

// https://www.pcg-random.org/
uint randInt(uint _v) {
	uint state = _v * 747796405u + 2891336453u;
	uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return (word >> 22u) ^ word;
}

float randFloat(uint _v) {
	
	return ldexp(randInt(_v), -32);
	
}

// https://graphics.pixar.com/library/OrthonormalB/paper.pdf
void orthonormalBasis(vec3 _n, out vec3 _b1, out vec3 _b2) {
	
	float sign = _n.z >= 0.0? 1.0 : -1.0;
	float a = -1.0 / (sign + _n.z);
	float b = _n.x * _n.y * a;
	_b1 = vec3(1.0 + sign * _n.x * _n.x * a, sign * b, -sign * _n.x);
	_b2 = vec3(b, sign + _n.y * _n.y * a, -_n.y);
	
}

// https://github.com/keijiro/KinoBloom/blob/master/Assets/Kino/Bloom/Shader/Bloom.cginc
vec4 karisAverage(vec4 _s1, vec4 _s2, vec4 _s3, vec4 _s4) {
	
	float s1w = 1.0 / (luma(_s1.rgb) + 1.0);
	float s2w = 1.0 / (luma(_s2.rgb) + 1.0);
	float s3w = 1.0 / (luma(_s3.rgb) + 1.0);
	float s4w = 1.0 / (luma(_s4.rgb) + 1.0);
	float one_div_wsum = 1.0 / (s1w + s2w + s3w + s4w);
	
	return (_s1 * s1w + _s2 * s2w + _s3 * s3w + _s4 * s4w) * one_div_wsum;
	
}

// Fast invertible tonemapper, useful for MSAA resolve

vec3 tonemap(vec3 _c) {
	
	return _c * rcp(max(_c.r, max(_c.g, _c.b)) + 1.0);
	
}

vec3 tonemapWithWeight(vec3 _c, float _w) {
	
	return _c * (_w * rcp(max(_c.r, max(_c.g, _c.b)) + 1.0));
	
}

vec3 tonemapInvert(vec3 _c) {
	
	return _c * rcp(1.0 - max(_c.r, max(_c.g, _c.b)));
	
}

#endif //UTILS_GLSL

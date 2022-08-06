// Generic commonly used functions

#ifndef UTILS_GLSL
#define UTILS_GLSL

// Useful primitives

float rcp(float _v) {
	
	return 1.0 / _v;
	
}

float2 rcp(float2 _v) {
	
	return float2(1.0) / _v;
	
}

float3 rcp(float3 _v) {
	
	return float3(1.0) / _v;
	
}

float2 saturate(float2 _v) {
	
	return clamp(_v, 0.0, 1.0);
	
}

uint divRoundUp(uint _val, uint _div) {
	
	return (_val - 1) / _div + 1;
	
};

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

float distanceSq(float2 _a, float2 _b) {
	
	float2 d = _a - _b;
	return dot(d, d);
	
}

// Conversion from linear RGB to sRGB
// http://www.java-gaming.org/topics/fast-srgb-conversion-glsl-snippet/37583/view.html
float3 srgbEncode(float3 _color) {
	
	float r = _color.r < 0.0031308 ? 12.92 * _color.r : 1.055 * pow(_color.r, 1.0/2.4) - 0.055;
	float g = _color.g < 0.0031308 ? 12.92 * _color.g : 1.055 * pow(_color.g, 1.0/2.4) - 0.055;
	float b = _color.b < 0.0031308 ? 12.92 * _color.b : 1.055 * pow(_color.b, 1.0/2.4) - 0.055;
	return float3(r, g, b);
	
}

// Conversion from sRGB to linear RGB
float3 srgbDecode(float3 _color) {
	
	float r = _color.r < 0.04045 ? (1.0 / 12.92) * _color.r : pow((_color.r + 0.055) * (1.0 / 1.055), 2.4);
	float g = _color.g < 0.04045 ? (1.0 / 12.92) * _color.g : pow((_color.g + 0.055) * (1.0 / 1.055), 2.4);
	float b = _color.b < 0.04045 ? (1.0 / 12.92) * _color.b : pow((_color.b + 0.055) * (1.0 / 1.055), 2.4);
	return float3(r, g, b);
	
}

float luminance(float3 _color) {
	
	float3 W = float3(0.2125, 0.7154, 0.0721);
	return dot(_color, W);
	
}

// https://github.com/hughsk/glsl-luma/blob/master/index.glsl
float luma(float3 _color) {
	
	float3 W = float3(0.299, 0.587, 0.114);
	return dot(_color, W);
	
}

// Create a bitmask with n lowest bits set
uint bitmask(uint _n) {
	
	if (_n == 32u)
		return -1u;
	else
		return (1u << _n) - 1u;
	
}

// Retrieve uint16 halves of a uint

uint uintLower(uint _n) {
	
	return _n & bitmask(16);
	
}

uint uintUpper(uint _n) {
	
	return _n >> 16u;
	
}

// Create a uint out of two uint16s
uint uintFromuint16(uint2 _val) {
	
	return (_val[1] << 16u) | _val[0];
	
}

// Create two uint16s out of a uint
uint2 uint16Fromuint(uint _val) {
	
	return uint2(uintLower(_val), uintUpper(_val));
	
}

#define U16FROMU32(_v) \
	uint2((_v) & 0xffffu, (_v) >> 16u)

// https://fgiesen.wordpress.com/2009/12/13/decoding-morton-codes/
uint mortonCompact1By1(uint _x) {
	
	_x &= 0x55555555u;                    // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
	_x = (_x ^ (_x >>  1)) & 0x33333333u; // x = --fe --dc --ba --98 --76 --54 --32 --10
	_x = (_x ^ (_x >>  2)) & 0x0f0f0f0fu; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
	_x = (_x ^ (_x >>  4)) & 0x00ff00ffu; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
	_x = (_x ^ (_x >>  8)) & 0x0000ffffu; // x = ---- ---- ---- ---- fedc ba98 7654 3210
	return _x;
	
}

uint2 mortonOrder(uint _id) {
	
	return uint2(mortonCompact1By1(_id), mortonCompact1By1(_id >> 1));
	
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

uint2 randInt2d(uint2 _v) {
	
	_v = _v * 1664525u + 1013904223u;
	
	_v.x += _v.y * 1664525u;
	_v.y += _v.x * 1664525u;
	
	_v = _v ^ (_v >> 16u);
	
	_v.x += _v.y * 1664525u;
	_v.y += _v.x * 1664525u;
	
	_v = _v ^ (_v >> 16u);
	
	return _v;
	
}

// https://graphics.pixar.com/library/OrthonormalB/paper.pdf
void orthonormalBasis(float3 _n, out float3 _b1, out float3 _b2) {
	
	float sign = _n.z >= 0.0? 1.0 : -1.0;
	float a = -1.0 / (sign + _n.z);
	float b = _n.x * _n.y * a;
	_b1 = float3(1.0 + sign * _n.x * _n.x * a, sign * b, -sign * _n.x);
	_b2 = float3(b, sign + _n.y * _n.y * a, -_n.y);
	
}

// https://github.com/keijiro/KinoBloom/blob/master/Assets/Kino/Bloom/Shader/Bloom.cginc
float4 karisAverage(float4 _s1, float4 _s2, float4 _s3, float4 _s4) {
	
	float s1w = 1.0 / (luma(_s1.rgb) + 1.0);
	float s2w = 1.0 / (luma(_s2.rgb) + 1.0);
	float s3w = 1.0 / (luma(_s3.rgb) + 1.0);
	float s4w = 1.0 / (luma(_s4.rgb) + 1.0);
	float one_div_wsum = 1.0 / (s1w + s2w + s3w + s4w);
	
	return (_s1 * s1w + _s2 * s2w + _s3 * s3w + _s4 * s4w) * one_div_wsum;
	
}

// Fast invertible tonemapper, useful for MSAA resolve

float3 tonemap(float3 _c) {
	
	return _c * rcp(max(_c.r, max(_c.g, _c.b)) + 1.0);
	
}

float3 tonemapWithWeight(float3 _c, float _w) {
	
	return _c * (_w * rcp(max(_c.r, max(_c.g, _c.b)) + 1.0));
	
}

float3 tonemapInvert(float3 _c) {
	
	return _c * rcp(1.0 - max(_c.r, max(_c.g, _c.b)));
	
}

// Octahedral normal encoding
// https://www.shadertoy.com/view/Mtfyzl

const uint NormalOctWidth = 16;

uint octEncode(float3 _vec) {
	
	uint mu = (1u << NormalOctWidth) - 1u;
	
	_vec /= abs(_vec.x) + abs(_vec.y) + abs(_vec.z);
	_vec.xy = (_vec.z >= 0.0)? _vec.xy : (1.0 - abs(_vec.yx)) * sign(_vec.xy);
	float2 v = 0.5 + 0.5 * _vec.xy;
	
	uint2 d = uint2(floor(v * float(mu) + 0.5));
	return (d.y << NormalOctWidth) | d.x;
	
}

float3 octDecode(uint _oct) {
	
	uint mu = (1u << NormalOctWidth) - 1u;
	
	uint2 d = uint2(_oct, _oct >> NormalOctWidth) & mu;
	float2 v = float2(d) / float(mu);
	
	v = v * 2.0 - 1.0;
	float3 norm = float3(v, 1.0 - abs(v.x) - abs(v.y));
	float t = max(-norm.z, 0.0);
	norm.x += (norm.x > 0.0)? -t : t;
	norm.y += (norm.y > 0.0)? -t : t;
	
	return normalize(norm);
	
}

#endif //UTILS_GLSL

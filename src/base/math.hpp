#pragma once

#include <concepts>
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/ext/vector_int2.hpp"
#include "glm/ext/vector_int3.hpp"
#include "glm/ext/vector_int4.hpp"
#include "glm/ext/vector_uint2.hpp"
#include "glm/ext/vector_uint3.hpp"
#include "glm/ext/vector_uint4.hpp"
#include "glm/gtc/type_precision.hpp"
#include "glm/ext/vector_common.hpp"
#include "glm/ext/matrix_float3x3.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "base/types.hpp"

namespace minote::base {

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::ivec2;
using glm::ivec3;
using glm::ivec4;
using glm::uvec2;
using glm::uvec3;
using glm::uvec4;
using glm::u16vec4;
using glm::mat3;
using glm::mat4;
using glm::radians;
using glm::clamp;
using glm::abs;
using glm::min;
using glm::max;
using glm::normalize;
using glm::length;
using glm::cross;
using glm::dot;
using glm::infinitePerspective;

// True modulo operation (as opposed to remainder, which is operator% in C++.)
// The result is always positive and does not flip direction at zero.
// Example:
//  5 mod 4 = 1
//  4 mod 4 = 0
//  3 mod 4 = 3
//  2 mod 4 = 2
//  1 mod 4 = 1
//  0 mod 4 = 0
// -1 mod 4 = 3
// -2 mod 4 = 2
// -3 mod 4 = 1
// -4 mod 4 = 0
// -5 mod 4 = 3
template<std::integral T>
constexpr auto tmod(T num, T div) { return num % div + (num % div < 0) * div; }

// Matrix generation shorthands

template<typename T = f32, glm::qualifier Q = glm::qualifier::defaultp>
constexpr auto make_translate(glm::vec<3, T, Q> v) -> glm::mat<4, 4, T, Q> {
	return glm::translate(glm::mat<4, 4, T, Q>(1), v);
}

template<typename T = f32, glm::qualifier Q = glm::qualifier::defaultp>
constexpr auto make_rotate(T angle, glm::vec<3, T, Q> v) -> glm::mat<4, 4, T, Q> {
	return glm::rotate(glm::mat<4, 4, T, Q>(1), angle, v);
}

template<typename T = f32, glm::qualifier Q = glm::qualifier::defaultp>
constexpr auto make_scale(glm::vec<3, T, Q> v) -> glm::mat<4, 4, T, Q> {
	return glm::scale(glm::mat<4, 4, T, Q>(1), v);
}

namespace literals {

constexpr float operator""_m(unsigned long long int val) { return float(val) * 0.001f; }
constexpr float operator""_m(long double val) { return float(val) * 0.001f; }

constexpr float operator""_km(unsigned long long int val) { return float(val); }
constexpr float operator""_km(long double val) { return float(val); }

constexpr float operator""_deg(unsigned long long int val) { return radians(float(val)); }
constexpr float operator""_deg(long double val) { return radians(float(val)); }

}

}

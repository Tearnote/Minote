// Minote - base/math.hpp
// cmath replacement, extended with vector and matrix types from the GLM library

#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/trigonometric.hpp>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>
#include <glm/ext/scalar_common.hpp>
#include <glm/ext/vector_common.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/color_space.hpp>
#include <glm/gtc/type_precision.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "base/concept.hpp"
#include "base/util.hpp"

namespace minote {

// *** Base types ***

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::ivec2;
using glm::ivec3;
using glm::ivec4;
using glm::uvec2;
using glm::uvec3;
using glm::uvec4;
using glm::mat4;
using glm::u8vec2;
using glm::u8vec3;
using glm::u8vec4;
using color3 = vec3;
using color4 = vec4;

// *** Constants ***

// A more correct replacement for pi.
// See: https://tauday.com/
template<floating_point T>
constexpr T Tau_v = 6.283185307179586476925286766559005768L;
constexpr auto Tau = Tau_v<f64>;

// *** Scalar / component-wise operations ***

using glm::abs;
using glm::sign;
using glm::floor;
using glm::trunc;
using glm::round;
using glm::ceil;
using glm::fract;
using glm::min;
using glm::max;
using glm::clamp;
using glm::repeat;
using glm::mix;
using glm::step;
using glm::smoothstep;
using glm::fma;
using glm::ldexp;
using glm::pow;
using glm::exp;
using glm::log;
using glm::exp2;
using glm::log2;
using glm::sqrt;
using glm::radians;
using glm::degrees;
using glm::sin;
using glm::cos;
using glm::tan;

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
template<integral T>
constexpr auto tmod(T num, T div) { return num % div + (num % div < 0) * div; }

// *** Vector operations ***

using glm::length;
using glm::distance;
using glm::dot;
using glm::cross;
using glm::normalize;
using glm::reflect;

// *** Matrix generation ***

using glm::perspective;
using glm::ortho;
using glm::lookAt;

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

// *** Matrix transforms ***

using glm::transpose;
using glm::inverse;
using glm::inverseTranspose;
using glm::translate;
using glm::rotate;
using glm::scale;

// *** Color operations ***

using glm::convertLinearToSRGB;
using glm::convertSRGBToLinear;

// *** Raw value passing ***

using glm::value_ptr;

// *** Compound types ***

// Axis-aligned bounding box. Contains position and size extending in positive
// direction of each axis. Parametrized by number of dimensions Dim = 2 or 3
// and underlying type T = i32 (size becomes u32) or f32.
template<int Dim, typename T>
struct AABB;

template<>
struct AABB<2, i32> {

	ivec2 pos;
	uvec2 size;

	auto operator==(AABB<2, i32> const& other) const -> bool = default;
	auto operator!=(AABB<2, i32> const& other) const -> bool = default;

	[[nodiscard]]
	auto zero() const -> bool {
		return pos.x == 0 && pos.y == 0 && size.x == 0 && size.y == 0;
	}

};

template<>
struct AABB<2, f32> {

	vec2 pos;
	vec2 size;

	auto operator==(AABB<2, f32> const& other) const -> bool = default;
	auto operator!=(AABB<2, f32> const& other) const -> bool = default;

	[[nodiscard]]
	auto zero() const -> bool {
		return pos.x == 0.0f && pos.y == 0.0f && size.x == 0.0f && size.y == 0.0f;
	}

};

template<>
struct AABB<3, i32> {

	ivec3 pos;
	uvec3 size;

	auto operator==(AABB<3, i32> const& other) const -> bool = default;
	auto operator!=(AABB<3, i32> const& other) const -> bool = default;

	[[nodiscard]]
	auto zero() const -> bool {
		return pos.x == 0 && pos.y == 0 && pos.z == 0 &&
			size.x == 0 && size.y == 0 && size.z == 0;
	}

};

template<>
struct AABB<3, f32> {

	vec3 pos;
	vec3 size;

	auto operator==(AABB<3, f32> const& other) const -> bool = default;
	auto operator!=(AABB<3, f32> const& other) const -> bool = default;

	[[nodiscard]]
	auto zero() const -> bool {
		return pos.x == 0.0f && pos.y == 0.0f && pos.z == 0.0f &&
			size.x == 0.0f && size.y == 0.0f && size.z == 0.0f;
	}

};

}

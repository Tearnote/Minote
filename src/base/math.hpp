/**
 * cmath replacement extended with vector and matrix types
 * @file
 * Partial scope import of the GLM library.
 */

#pragma once

#include <functional>
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
#include "base/util.hpp"

namespace minote {

// Base types
using glm::u8;
using glm::u16;
using glm::u32;
using glm::i32;
using glm::f32;
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

// Constants
/**
 * A more correct replacement for pi.
 * @see https://tauday.com/
 */
template<FloatingPoint T>
constexpr T Tau_v = 6.283185307179586476925286766559005768L;
constexpr auto Tau = Tau_v<double>;

// Scalar / component-wise operations
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

/**
 * True modulo operation (as opposed to remainder, which is operator% in C++.)
 * @param num Value
 * @param div Modulo divisor
 * @return Result of modulo (always positive)
 */
template<Integral T>
constexpr auto tmod(T num, T div) { return num % div + (num % div < 0) * div; }

// Vector operations
using glm::length;
using glm::distance;
using glm::dot;
using glm::cross;
using glm::normalize;
using glm::reflect;

// Matrix generation
using glm::perspective;
using glm::ortho;

template<typename T, glm::qualifier Q>
constexpr auto make_translate = std::bind(glm::translate<T, Q>, mat4(1.0f),
	std::placeholders::_1);

template<typename T, glm::qualifier Q>
constexpr auto make_rotate = std::bind(glm::rotate<T, Q>, mat4(1.0f),
	std::placeholders::_1, std::placeholders::_2);

template<typename T, glm::qualifier Q>
constexpr auto make_scale = std::bind(glm::scale<T, Q>, mat4(1.0f),
	std::placeholders::_1);

// Matrix transforms
using glm::transpose;
using glm::inverse;
using glm::inverseTranspose;
using glm::translate;
using glm::rotate;
using glm::scale;
using glm::lookAt;

// Color operations
using glm::convertLinearToSRGB;
using glm::convertSRGBToLinear;

// Raw value passing
using glm::value_ptr;

// Compound types

/**
 * Axis-aligned bounding box, defined by position and size (extending
 * in positive direction of each axis.)
 * @tparam Dim Number of dimensions (2 or 3)
 * @tparam T Underlying type, i32 (size is u32) or f32
 */
template<int Dim, typename T>
struct AABB;

template<>
struct AABB<2, i32> {

	ivec2 pos = {0, 0};
	uvec2 size = {0, 0};

	auto operator==(AABB<2, i32> const& other) const -> bool
	{
		return pos == other.pos && size == other.size;
	}

	auto operator!=(AABB<2, i32> const& other) const -> bool
	{
		return !(other == *this);
	}

	[[nodiscard]]
	auto zero() const -> bool
	{
		return pos.x == 0 && pos.y == 0 && size.x == 0 && size.y == 0;
	}

};

template<>
struct AABB<3, i32> {

	ivec3 pos = {0, 0, 0};
	uvec3 size = {0, 0, 0};

	auto operator==(AABB<3, i32> const& other) const -> bool
	{
		return pos == other.pos && size == other.size;
	}

	auto operator!=(AABB<3, i32> const& other) const -> bool
	{
		return !(other == *this);
	}

	[[nodiscard]]
	auto zero() const -> bool
	{
		return pos.x == 0 && pos.y == 0 && pos.z == 0 &&
			size.x == 0 && size.y == 0 && size.z == 0;
	}

};

template<>
struct AABB<2, f32> {

	vec2 pos = {0.0f, 0.0f};
	vec2 size = {0.0f, 0.0f};

	auto operator==(AABB<2, f32> const& other) const -> bool
	{
		return pos == other.pos && size == other.size;
	}

	auto operator!=(AABB<2, f32> const& other) const -> bool
	{
		return !(other == *this);
	}

	[[nodiscard]]
	auto zero() const -> bool
	{
		return pos.x == 0.0f && pos.y == 0.0f && size.x == 0.0f && size.y == 0.0f;
	}

};

template<>
struct AABB<3, f32> {

	vec3 pos = {0, 0, 0};
	vec3 size = {0, 0, 0};

	auto operator==(AABB<3, f32> const& other) const -> bool
	{
		return pos == other.pos && size == other.size;
	}

	auto operator!=(AABB<3, f32> const& other) const -> bool
	{
		return !(other == *this);
	}

	[[nodiscard]]
	auto zero() const -> bool
	{
		return pos.x == 0.0f && pos.y == 0.0f && pos.z == 0.0f &&
			size.x == 0.0f && size.y == 0.0f && size.z == 0.0f;
	}

};

}

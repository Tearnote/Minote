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
#include <glm/gtc/type_ptr.hpp>
#include "base/util.hpp"

namespace minote {

// Base types
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using ivec2 = glm::ivec2;
using ivec3 = glm::ivec3;
using ivec4 = glm::ivec4;
using mat4 = glm::mat4;
using color3 = glm::vec3;
using color4 = glm::vec4;

// Constants
constexpr color4 Clear4 = {1.0f, 1.0f, 1.0f, 0.0f};

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

// Useful concepts
template<typename T>
concept GLSLType =
	std::is_same_v<T, float>
	|| std::is_same_v<T, int>
	|| std::is_same_v<T, vec2>
	|| std::is_same_v<T, vec3>
	|| std::is_same_v<T, vec4>
	|| std::is_same_v<T, ivec2>
	|| std::is_same_v<T, ivec3>
	|| std::is_same_v<T, ivec4>
	|| std::is_same_v<T, mat4>;

}

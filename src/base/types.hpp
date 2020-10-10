/**
 * POD types for dealing with coordinates, sizes and colors
 * @file
 */

#pragma once

#include <type_traits>
#include <array>
#include "base/util.hpp"

namespace minote {

/// A two-element vector type
template<Arithmetic T>
struct Vec2 {

	union {
		T x = 0;
		T r;
		T u;
	};
	union {
		T y = 0;
		T g;
		T v;
	};

	/**
	 * Convert to std::array, for passing as continuous memory.
	 * @return std::array version of the vector
	 */
	[[nodiscard]]
	constexpr auto arr() -> std::array<T, 2> { return {x, y}; }
	[[nodiscard]]
	constexpr auto arr() const -> std::array<const T, 2> { return {x, y}; }

	constexpr auto operator==(const Vec2<T> other) const -> bool
	{
		return x == other.x && y == other.y;
	}
};

// We need this to be true for the union to be valid
static_assert(std::is_standard_layout_v<Vec2<int>>);

using point2i = Vec2<int>; ///< Integer position in 2D space
using size2i = Vec2<int>; ///< Integer size in 2D space
using point2f = Vec2<float>; ///< Floating-point position in 2D space
using size2f = Vec2<float>; ///< Floating-point size in 2D space

/// A three-element vector type
template<Arithmetic T>
struct Vec3 {

	union {
		T x = 0;
		T r;
		T u;
	};
	union {
		T y = 0;
		T g;
		T v;
	};
	union {
		T z = 0;
		T b;
		T s;
	};

	/**
	 * Convert to std::array, for passing as continuous memory.
	 * @return std::array version of the vector
	 */
	[[nodiscard]]
	constexpr auto arr() -> std::array<T, 3> { return {x, y, z}; }
	[[nodiscard]]
	constexpr auto arr() const -> std::array<const T, 3> { return {x, y, z}; }

	/**
	 * Interpret the vector as a color and convert it from gamma to linear
	 * colorspace.
	 * @return Linear space color
	 */
	[[nodiscard]]
	constexpr auto toLinear() const -> Vec3<T> requires FloatingPoint<T>
	{
		return {std::pow(r, 2.2f), std::pow(g, 2.2f), std::pow(b, 2.2f)};
	};

	/**
	 * Interpret the vector as a color and convert it from linear to gamma
	 * colorspace.
	 * @return Gamma space color
	 */
	[[nodiscard]]
	constexpr auto toGamma() const -> Vec3<T> requires FloatingPoint<T>
	{
		return {std::pow(r, 0.4545f), std::pow(g, 0.4545f),
		        std::pow(b, 0.4545f)};
	};

	constexpr auto operator==(const Vec3<T> other) const -> bool
	{
		return x == other.x && y == other.y && z == other.z;
	}

};

// We need this to be true for the union to be valid
static_assert(std::is_standard_layout_v<Vec3<int>>);

using point3i = Vec3<int>; ///< Integer position in 2D space
using size3i = Vec3<int>; ///< Integer size in 2D space
using point3f = Vec3<float>; ///< Floating-point position in 2D space
using size3f = Vec3<float>; ///< Floating-point size in 2D space
using color3 = Vec3<float>; ///< RGB color value

/// A four-element vector type
template<Arithmetic T>
struct Vec4 {

	union {
		T x = 0;
		T r;
		T u;
	};
	union {
		T y = 0;
		T g;
		T v;
	};
	union {
		T z = 0;
		T b;
		T s;
	};
	union {
		T w = 0;
		T a;
		T t;
	};

	/**
	 * Convert to std::array, for passing as continuous memory.
	 * @return std::array version of the vector
	 */
	[[nodiscard]]
	constexpr auto arr() -> std::array<T, 4> { return {x, y, z, w}; }
	[[nodiscard]]
	constexpr auto arr() const -> std::array<const T, 4> { return {x, y, z, w}; }

	/**
	 * Interpret the vector as a color and convert it from gamma to linear
	 * colorspace.
	 * @return Linear space color
	 */
	[[nodiscard]]
	constexpr auto toLinear() const -> Vec4<T> requires FloatingPoint<T>
	{
		return {std::pow(r, 2.2f), std::pow(g, 2.2f), std::pow(b, 2.2f), a};
	};

	/**
	 * Interpret the vector as a color and convert it from linear to gamma
	 * colorspace.
	 * @return Gamma space color
	 */
	[[nodiscard]]
	constexpr auto toGamma() const -> Vec4<T> requires FloatingPoint<T>
	{
		return {std::pow(r, 0.4545f), std::pow(g, 0.4545f),
		        std::pow(b, 0.4545f), a};
	};

	constexpr auto operator==(const Vec4<T> other) const -> bool
	{
		return x == other.x && y == other.y && z == other.z && w == other.w;
	}

};

// We need this to be true for the union to be valid
static_assert(std::is_standard_layout_v<Vec4<int>>);

using point4f = Vec4<float>; ///< 3D vector with a w coordinate
using color4 = Vec4<float>; ///< RGBA color value

constexpr color3 White3 = {1.0f, 1.0f, 1.0f};
constexpr color4 White4 = {1.0f, 1.0f, 1.0f, 1.0f};
constexpr color3 Black3 = {0.0f, 0.0f, 0.0f};
constexpr color4 Black4 = {0.0f, 0.0f, 0.0f, 1.0f};
constexpr color4 Clear4 = {1.0f, 1.0f, 1.0f, 0.0f};

}

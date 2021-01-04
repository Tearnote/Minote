#pragma once

#include <concepts>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <fmt/format.h>
#include "base/types.hpp"

namespace minote::base {

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

// Axis-aligned bounding box. Contains position and size extending in positive
// direction of each axis. Parametrized by number of dimensions Dim = 2 or 3
// and underlying type T = i32 (size becomes u32) or f32.
template<int Dim, typename T>
struct AABB;

template<>
struct AABB<2, i32> {

	glm::ivec2 pos;
	glm::uvec2 size;

	auto operator==(AABB<2, i32> const& other) const -> bool = default;
	auto operator!=(AABB<2, i32> const& other) const -> bool = default;

	[[nodiscard]]
	auto zero() const -> bool {
		return pos.x == 0 && pos.y == 0 && size.x == 0 && size.y == 0;
	}

};

template<>
struct AABB<2, f32> {

	glm::vec2 pos;
	glm::vec2 size;

	auto operator==(AABB<2, f32> const& other) const -> bool = default;
	auto operator!=(AABB<2, f32> const& other) const -> bool = default;

	[[nodiscard]]
	auto zero() const -> bool {
		return pos.x == 0.0f && pos.y == 0.0f && size.x == 0.0f && size.y == 0.0f;
	}

};

template<>
struct AABB<3, i32> {

	glm::ivec3 pos;
	glm::uvec3 size;

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

	glm::vec3 pos;
	glm::vec3 size;

	auto operator==(AABB<3, f32> const& other) const -> bool = default;
	auto operator!=(AABB<3, f32> const& other) const -> bool = default;

	[[nodiscard]]
	auto zero() const -> bool {
		return pos.x == 0.0f && pos.y == 0.0f && pos.z == 0.0f &&
			size.x == 0.0f && size.y == 0.0f && size.z == 0.0f;
	}

};

struct vec_formatter {

	constexpr auto parse(fmt::format_parse_context& ctx) {
		auto it = ctx.begin();
		auto end = ctx.end();
		while (true) {
			if (it == end || *it == '}')
				break;
			if (*it == 's') {
				size = true;
				++it;
				continue;
			}
			throw fmt::format_error("invalid format");
		}
		return it;
	}

protected:

	bool size{false};

};

template<typename T>
struct vec2_formatter: vec_formatter {

	template<typename FormatContext>
	auto format(const T& val, FormatContext& ctx) {
		if (size)
			return fmt::format_to(ctx.out(), "{}x{}", val.x, val.y);
		else
			return fmt::format_to(ctx.out(), "({}, {})", val.x, val.y);
	}

};

template<typename T>
struct vec3_formatter: vec_formatter {

	template<typename FormatContext>
	auto format(const T& val, FormatContext& ctx) {
		if (size)
			return fmt::format_to(ctx.out(), "{}x{}", val.x, val.y, val.z);
		else
			return fmt::format_to(ctx.out(), "({}, {})", val.x, val.y, val.z);
	}

};

template<typename T>
struct vec4_formatter: vec_formatter {

	template<typename FormatContext>
	auto format(const T& val, FormatContext& ctx) {
		if (size)
			return fmt::format_to(ctx.out(), "{}x{}", val.x, val.y, val.z, val.w);
		else
			return fmt::format_to(ctx.out(), "({}, {}, {}, {})", val.x, val.y, val.z, val.w);
	}

};

template<typename T>
struct aabb_formatter {

	constexpr auto parse(fmt::format_parse_context& ctx) {
		auto it = ctx.begin();
		auto end = ctx.end();
		if (it != end && *it != '}')
			throw fmt::format_error("invalid format");
		return it;
	}

	template<typename FormatContext>
	auto format(const T& val, FormatContext& ctx) {
		return fmt::format_to(ctx.out(), "({}, {})",
			val.pos, val.m_size);
	}

};

}

template<>
struct fmt::formatter<glm::vec2>: minote::base::vec2_formatter<glm::vec2> {};
template<>
struct fmt::formatter<glm::ivec2>: minote::base::vec2_formatter<glm::ivec2> {};
template<>
struct fmt::formatter<glm::uvec2>: minote::base::vec2_formatter<glm::uvec2> {};
template<>
struct fmt::formatter<glm::u8vec2>: minote::base::vec2_formatter<glm::u8vec2> {};
template<>
struct fmt::formatter<glm::vec3>: minote::base::vec3_formatter<glm::vec3> {};
template<>
struct fmt::formatter<glm::ivec3>: minote::base::vec3_formatter<glm::ivec3> {};
template<>
struct fmt::formatter<glm::uvec3>: minote::base::vec3_formatter<glm::uvec3> {};
template<>
struct fmt::formatter<glm::u8vec3>: minote::base::vec3_formatter<glm::u8vec3> {};
template<>
struct fmt::formatter<glm::vec4>: minote::base::vec4_formatter<glm::vec4> {};
template<>
struct fmt::formatter<glm::ivec4>: minote::base::vec4_formatter<glm::ivec4> {};
template<>
struct fmt::formatter<glm::uvec4>: minote::base::vec4_formatter<glm::uvec4> {};
template<>
struct fmt::formatter<glm::u8vec4>: minote::base::vec4_formatter<glm::u8vec4> {};
template<>
struct fmt::formatter<minote::base::AABB<2, minote::base::i32>>:
	minote::base::aabb_formatter<minote::base::AABB<2, minote::base::i32>> {};
template<>
struct fmt::formatter<minote::base::AABB<2, minote::base::u32>>:
	minote::base::aabb_formatter<minote::base::AABB<2, minote::base::u32>> {};
template<>
struct fmt::formatter<minote::base::AABB<3, minote::base::i32>>:
	minote::base::aabb_formatter<minote::base::AABB<3, minote::base::i32>> {};
template<>
struct fmt::formatter<minote::base::AABB<3, minote::base::u32>>:
	minote::base::aabb_formatter<minote::base::AABB<3, minote::base::u32>> {};

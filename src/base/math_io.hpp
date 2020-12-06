// Minote - base/math_io.hpp
// Formatting of math types

#pragma once

#include "base/math.hpp"
#include "base/io.hpp"

namespace minote {

template<typename T>
struct vec2_formatter: simple_formatter {

	template<typename FormatContext>
	auto format(const T& val, FormatContext& ctx) {
		return format_to(ctx.out(), "({}, {})",
			val.x, val.y);
	}

};

template<typename T>
struct vec3_formatter: simple_formatter {

	template<typename FormatContext>
	auto format(const T& val, FormatContext& ctx) {
		return format_to(ctx.out(), "({}, {}, {})",
			val.x, val.y, val.z);
	}

};

template<typename T>
struct vec4_formatter: simple_formatter {

	template<typename FormatContext>
	auto format(const T& val, FormatContext& ctx) {
		return format_to(ctx.out(), "({}, {}, {}, {})",
			val.x, val.y, val.z, val.w);
	}

};

template<typename T>
struct aabb_formatter: simple_formatter {

	template<typename FormatContext>
	auto format(const T& val, FormatContext& ctx) {
		return format_to(ctx.out(), "({}, {})",
			val.pos, val.size);
	}

};

}

template<>
struct fmt::formatter<minote::vec2>: minote::vec2_formatter<minote::vec2> {};
template<>
struct fmt::formatter<minote::ivec2>: minote::vec2_formatter<minote::ivec2> {};
template<>
struct fmt::formatter<minote::uvec2>: minote::vec2_formatter<minote::uvec2> {};
template<>
struct fmt::formatter<minote::u8vec2>: minote::vec2_formatter<minote::u8vec2> {};
template<>
struct fmt::formatter<minote::vec3>: minote::vec3_formatter<minote::vec3> {};
template<>
struct fmt::formatter<minote::ivec3>: minote::vec3_formatter<minote::ivec3> {};
template<>
struct fmt::formatter<minote::uvec3>: minote::vec3_formatter<minote::uvec3> {};
template<>
struct fmt::formatter<minote::u8vec3>: minote::vec3_formatter<minote::u8vec3> {};
template<>
struct fmt::formatter<minote::vec4>: minote::vec4_formatter<minote::vec4> {};
template<>
struct fmt::formatter<minote::ivec4>: minote::vec4_formatter<minote::ivec4> {};
template<>
struct fmt::formatter<minote::uvec4>: minote::vec4_formatter<minote::uvec4> {};
template<>
struct fmt::formatter<minote::u8vec4>: minote::vec4_formatter<minote::u8vec4> {};
template<>
struct fmt::formatter<minote::AABB<2, minote::i32>>: minote::aabb_formatter<minote::AABB<2, minote::i32>> {};
template<>
struct fmt::formatter<minote::AABB<2, minote::u32>>: minote::aabb_formatter<minote::AABB<2, minote::u32>> {};
template<>
struct fmt::formatter<minote::AABB<3, minote::i32>>: minote::aabb_formatter<minote::AABB<3, minote::i32>> {};
template<>
struct fmt::formatter<minote::AABB<3, minote::u32>>: minote::aabb_formatter<minote::AABB<3, minote::u32>> {};

#pragma once

#include <tuple>
#include <fmt/format.h>
#include "base/types.hpp"

namespace minote::base {

// Simple SemVer triple
using Version = std::tuple<u32, u32, u32>;

}

template<>
struct fmt::formatter<minote::base::Version> {

	constexpr auto parse(fmt::format_parse_context& ctx) {
		auto it = ctx.begin();
		auto end = ctx.end();
		if (it != end && *it != '}')
			throw fmt::format_error("invalid format");
		return it;
	}

	template<typename FormatContext>
	auto format(minote::base::Version const& ver, FormatContext& ctx) {
		return format_to(ctx.out(), "{}.{}.{}",
			std::get<0>(ver), std::get<1>(ver), std::get<2>(ver));
	}

};

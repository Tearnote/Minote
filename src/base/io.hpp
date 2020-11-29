// Minote - base/io.hpp
// Import of file, filesystem and formatting types

#pragma once

#include <filesystem>
#include <cstring>
#include <cstdio>
#include <fmt/format.h>
#include <fmt/chrono.h>

namespace minote {

using std::FILE;
using std::fopen;
using std::fclose;
using std::fflush;
using std::perror;
using std::strerror;
using std::filesystem::path;
using fmt::format;
using fmt::format_to;
using fmt::print;
using fmt::memory_buffer;

// Simple base for formatters that do not accept any parameters
struct simple_formatter {

	constexpr auto parse(fmt::format_parse_context& ctx)
	{
		auto it = ctx.begin();
		auto end = ctx.end();
		if (it != end && *it != '}')
			throw fmt::format_error("invalid format");
		return it;
	}

};

}

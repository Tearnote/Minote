#include "base/file.hpp"

#include <system_error>
#include <utility>
#include <cstdio>
#include <fmt/core.h>
#include "base/util.hpp"

namespace minote::base {

void file::open(std::filesystem::path const& path, char const* mode) {
	if (handle) close();

	pathStr = path.string();
	handle = std::fopen(pathStr.c_str(), mode);
	if (!handle)
		throw std::system_error{errno, std::generic_category(),
		                   fmt::format(R"(Failed to open "{}")", pathStr)};
}

void file::close() {
	if (!handle) return;

	if (!noClose) {
		auto const result = std::fclose(handle);
		if (result)
			throw std::system_error{errno, std::generic_category(),
			                   fmt::format(R"(Failed to flush file "{}" on closing)", pathStr)};
	}
	handle = nullptr;
}

void file::flush() {
	if (!handle) return;

	if (std::fflush(handle) == EOF)
		throw std::system_error{errno, std::generic_category(),
		                   fmt::format(R"(Failed to flush file "{}")", pathStr)};

}

file::file(file&& other) noexcept {
	*this = std::move(other);
}

auto file::operator=(file&& other) noexcept -> file& {
	close();

	pathStr = std::move(other.pathStr);
	handle = other.handle;
	other.handle = nullptr;

	return *this;
}

}

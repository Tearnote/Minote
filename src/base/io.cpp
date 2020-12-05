#include "base/io.hpp"

#include <system_error>
#include <cstdio>
#include "base/util.hpp"

namespace minote {

void file::open(path const& path, char const* mode) {
	ASSERT(!handle);

	pathStr = path.string();
	handle = std::fopen(pathStr.c_str(), mode);
	if (!handle)
		throw system_error{errno, std::generic_category(),
		                   format(R"(Failed to open "{}")", pathStr)};
}

void file::close() {
	if (!handle) return;

	if (!noClose) {
		auto const result = std::fclose(handle);
		if (result)
			throw system_error{errno, std::generic_category(),
			                   format(R"(Failed to flush file "{}" on closing)", pathStr)};
	}
	handle = nullptr;
}

void file::flush() {
	if (!handle) return;

	if (std::fflush(handle) == EOF)
		throw system_error{errno, std::generic_category(),
		                   format(R"(Failed to flush file "{}")", pathStr)};

}

file::file(file&& other) noexcept:
	pathStr{move(other.pathStr)} {

	handle = other.handle;
	other.handle = nullptr;

}

auto file::operator=(file&& other) noexcept -> file& {

	close();

	pathStr = move(other.pathStr);
	handle = other.handle;
	other.handle = nullptr;

	return *this;

}

}

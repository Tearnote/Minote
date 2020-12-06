// Minote - base/io.hpp
// Import of file, filesystem and formatting types

#pragma once

#include <system_error>
#include <filesystem>
#include <cstdio>
#include <fmt/format.h>
#include "base/string.hpp"

namespace minote {

// *** File IO ***

using std::filesystem::path;

// An exception thrown on file IO error
using std::system_error;

// RAII wrapper for C-style file functions. Throws system_error on errors. To avoid constructor
// and destructor throws, use open() and close() methods manually instead of relying on RAII.
struct file {

	// Create a null object, with no file attached.
	file() noexcept = default;

	// Create the object from an externally opened std::FILE. Ownership of the FILE is assumed.
	// If the FILE is special and should never be closed, set doNotClose to true.
	file(std::FILE* raw, string_view name, bool const doNotClose = false) noexcept:
		handle{raw}, pathStr{name}, noClose{doNotClose} {}

	// Create the object with an immediately attached file.
	file(path const& path, char const* mode) { open(path, mode); }

	// Close the attached file if open. May throw on the final buffer flush.
	~file() noexcept(false) { close(); }

	// Open the file with a given mode (fopen() mode string). Any previous attached file
	// is closed.
	void open(path const&, char const* mode);

	// Close the attached file if exists. May throw on the final buffer flush.
	void close();

	// Flush the buffer of attached file (if exists) to the storage device.
	void flush();

	// True if currently attached to a file.
	[[nodiscard]]
	auto isOpen() const { return handle == nullptr; }

	// Resolved path of the file.
	[[nodiscard]]
	auto where() const -> string_view { return pathStr; }

	operator bool() const { return isOpen(); }

	// Implicit conversion to C-style file handle. nullptr if no file attached.
	// Do not fclose() or freopen()!
	operator std::FILE*() { return handle; }

	// Moveable, not copyable
	file(file const&) = delete;
	auto operator=(file const&) -> file& = delete;
	file(file&&) noexcept;
	auto operator=(file&&) noexcept -> file&;

private:

	std::FILE* handle{nullptr}; // nullptr if no file attached
	string pathStr;
	bool const noClose{false}; // Special file that should not be fclosed

};

inline static file cout{stdout, "stdout", true};
inline static file cerr{stderr, "stderr", true};

// *** Text formatting and printing ***

// Return a new string with formatted text
using fmt::format;

// Write formatted text into a char container
using fmt::format_to;

// Prints formatted text to stdout or a file. Throws system_error on write error
using fmt::print;

// Generic container that uses static memory up to a size, and switches to dynamic above it.
// Useful for optimizing the average-case string length
using fmt::memory_buffer;

// Base for formatters that do not accept any parameters
struct simple_formatter {

	constexpr auto parse(fmt::format_parse_context& ctx) {
		auto it = ctx.begin();
		auto end = ctx.end();
		if (it != end && *it != '}')
			throw fmt::format_error("invalid format");
		return it;
	}

};

}

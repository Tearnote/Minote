#pragma once

#include <string_view>
#include <filesystem>
#include <string>
#include <cstdio>

namespace minote::base {

// RAII wrapper for C-style file functions. Throws std::system_error on errors. To avoid
// constructor and destructor throws, use open() and close() methods manually instead of relying
// on RAII.
struct file {

	// Create a null object, with no file attached.
	file() noexcept = default;

	// Create the object from an externally opened std::FILE. Ownership of the FILE is assumed.
	// If the FILE is special and should never be closed, set doNotClose to true.
	file(std::FILE* raw, std::string_view name, bool doNotClose = false) noexcept:
		handle(raw), pathStr(name), noClose(doNotClose) {}

	// Create the object with an immediately attached file.
	file(std::filesystem::path const& path, char const* mode) { open(path, mode); }

	// Close the attached file if open. May throw on the final buffer flush.
	~file() noexcept(false) { close(); }

	// Open the file with a given mode (fopen() mode string). Any previous attached file
	// is closed.
	void open(std::filesystem::path const& path, char const* mode);

	// Close the attached file if exists. May throw on the final buffer flush.
	void close();

	// Flush the buffer of attached file (if exists) to the storage device.
	void flush();

	// True if currently attached to a file.
	[[nodiscard]]
	auto isOpen() const { return handle == nullptr; }

	// Resolved path of the file.
	[[nodiscard]]
	auto where() const -> std::string_view { return pathStr; }

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

	std::FILE* handle = nullptr; // nullptr if no file attached
	std::string pathStr;
	bool noClose = false; // Special file that should not be fclosed

};

inline static auto cout = file(stdout, "stdout", true);
inline static auto cerr = file(stderr, "stderr", true);

}

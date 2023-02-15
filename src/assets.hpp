#pragma once

#include <concepts>
#include <span>
#include "util/string.hpp"

// Forward declaration
struct sqlite3;

namespace minote {

struct Assets {
	
	// Open the sqlite database containing game assets. File remains open
	// until this object is destroyed
	explicit Assets(string_view path);
	~Assets();
	
	// Iterate over the models table, and call the provided function on each row
	// Function arguments are name of the model, and raw bytestream as char array
	template<typename F>
	requires std::invocable<F, string_view, std::span<char const>>
	void loadModels(F func);
	
	Assets(Assets const&) = delete;
	auto operator=(Assets const&) -> Assets& = delete;
	
private:
	
	sqlite3* m_db = nullptr;
	string m_path;
	
};

}

#include "assets.tpp"

#pragma once

#include "util/concepts.hpp"
#include "util/string.hpp"
#include "util/span.hpp"

// Forward declaration
struct sqlite3;

namespace minote {

struct Assets {
	
	// Open the sqlite database containing game assets. File remains open
	// until this object is destroyed
	explicit Assets(string_view path);
	~Assets();
	
	// Iterate over the models table, and call the provided function on each row
	// Function arguments:
	// - name of the model
	// - raw bytestream as char array
	template<typename F>
	requires invocable<F, string_view, span<char const>>
	void loadModels(F func);
	
	Assets(Assets const&) = delete;
	auto operator=(Assets const&) -> Assets& = delete;
	
private:
	
	sqlite3* m_db = nullptr;
	string m_path;
	
};

}

#include "assets.tpp"

#pragma once

#include <string_view>
#include <concepts>
#include <string>
#include <span>

// Forward declaration
struct sqlite3;

namespace minote::game {

class Assets {

public:
	
	// Open the sqlite database containing game assets. File remains open
	// until this object is destroyed
	explicit Assets(std::string_view path);
	~Assets();
	
	// Iterate over the models table, and call the provided function on each row
	// Function arguments are name of the model, and raw bytestream as char array
	template<typename F>
	requires std::invocable<F, std::string_view, std::span<char const>>
	void loadModels(F func);
	
	// Not moveable, not copyable
	Assets(Assets const&) = delete;
	auto operator=(Assets const&) -> Assets& = delete;
	
private:
	
	sqlite3* m_db = nullptr;
	std::string m_path;
	
};

}

#include "game/assets.tpp"

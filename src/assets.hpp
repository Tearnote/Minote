#pragma once

#include <concepts>
#include <span>
#include "sqlite3.h"
#include "util/string.hpp"

namespace minote {

struct Assets {
	
	// Open the sqlite database containing game assets. File remains open
	// until this object is destroyed.
	explicit Assets(string_view path);
	~Assets();
	
	// Iterate over all rows in the models table, and call the provided function
	// on each model's data.
	template<typename F>
	requires std::invocable<F, string_view, std::span<char const>>
	void loadModels(F func);
	
	// Not moveable, not copyable
	Assets(Assets const&) = delete;
	auto operator=(Assets const&) -> Assets& = delete;
	
private:
	
	static constexpr auto Models_n = "models";
	
	sqlite3* m_db = nullptr;
	string m_path;
	
};

}

#include "assets.tpp"

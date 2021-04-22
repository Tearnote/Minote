#pragma once

#include <string_view>
#include <concepts>
#include <string>
#include <span>
#include "sqlite3.h"

namespace minote {

struct Assets {

	explicit Assets(std::string_view path);
	~Assets();

	template<typename F>
		requires std::invocable<F, std::string_view, std::span<char const>>
	void loadModels(F func);

	// Not moveable, not copyable
	Assets(Assets const&) = delete;
	auto operator=(Assets const&) -> Assets& = delete;
	Assets(Assets&&) noexcept = delete;
	auto operator=(Assets&&) noexcept -> Assets& = delete;

private:

	sqlite3* db = nullptr;
	std::string path;

};

}

#include "assets.tpp"

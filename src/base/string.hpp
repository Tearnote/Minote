// Minote - base/string.hpp
// String types and utilities

#pragma once

#include <string_view>
#include <string>
#include "base/util.hpp"

namespace minote {

using namespace std::string_view_literals;
using namespace std::string_literals;

using std::string;
using std::string_view;

// Resource ID. created from a string hashed at compile-time if possible
struct ID {

	// Hash string with FNV-1a
	explicit constexpr ID(string_view str): id{Basis} {
		std::for_each(str.begin(), str.end(), [this](auto ch){
			id ^= ch;
			id *= Prime;
		});
	}

	constexpr auto operator==(ID const&) const -> bool = default;
	constexpr auto operator!=(ID const&) const -> bool = default;
	constexpr auto operator+() const { return id; }

private:

	static constexpr u32 Prime{16777619u};
	static constexpr u32 Basis{2166136261u};

	u32 id;

};

// Guaranteed-constexpr string literal hash
consteval auto operator""_id(char const* str, size_t len) { return ID{{str, len}}; }

}

// Providing std::hash<ID>. The ID is already hashed, so this is an identity function
namespace std {

template<>
struct hash<minote::ID> {

	constexpr auto operator()(minote::ID const& id) const -> size_t { return +id; }

};

}

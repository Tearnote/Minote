#pragma once

#include <string_view>
#include <algorithm>
#include <cstddef>
#include "base/types.hpp"

namespace minote::base {

// Resource ID. created from a string hashed at compile-time if possible
struct ID {

	// Hash string with FNV-1a
	explicit constexpr ID(std::string_view str): id{Basis} {
		std::ranges::for_each(str, [this](auto ch){
			id ^= ch;
			id *= Prime;
		});
	}

	constexpr auto operator==(ID const&) const -> bool = default;
	constexpr auto operator!=(ID const&) const -> bool = default;
	constexpr auto operator+() const { return id; }

private:

	static constexpr u32 Prime = 16777619u;
	static constexpr u32 Basis = 2166136261u;

	u32 id;

};

namespace literals {

// Guaranteed-constexpr string literal hash
consteval auto operator ""_id(char const* str, size_t len) { return ID{{str, len}}; }

}

}

// Providing std::hash<ID>. The ID is already hashed, so this is an identity function
namespace std {

template<>
struct hash<minote::base::ID> {

	constexpr auto operator()(minote::base::ID const& id) const -> std::size_t { return +id; }

};

}

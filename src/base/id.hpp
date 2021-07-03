#pragma once

#include "base/container/string.hpp"
#include "base/types.hpp"

namespace minote::base {

// Resource ID. Created from a string, hashed at compile-time if possible.
struct ID {
	
	constexpr ID(): id(0u) {}
	
	// Hash string with FNV-1a
	explicit constexpr ID(string_view str): id(Basis) {
		
		for (auto ch: str) {
			
			id ^= ch;
			id *= Prime;
			
		}
		
	}
	
	constexpr auto operator==(ID const&) const -> bool = default;
	constexpr auto operator!=(ID const&) const -> bool = default;
	constexpr auto operator+() const { return id; }
	
private:
	
	static constexpr auto Prime = 16777619u;
	static constexpr auto Basis = 2166136261u;
	
	u32 id;
	
};

namespace literals {

// Guaranteed-constexpr string literal hash
consteval auto operator ""_id(char const* str, usize len) {
	
	return ID(string_view(str, len));
	
}

}

}

namespace std {

// Providing std::hash<ID>. The ID is already hashed, so this is an identity function
template<>
struct hash<minote::base::ID> {
	
	constexpr auto operator()(minote::base::ID const& id) const -> std::size_t { return +id; }
	
};

}

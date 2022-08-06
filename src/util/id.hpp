#pragma once

#include <type_traits>
#include "util/string.hpp"
#include "util/types.hpp"

namespace minote {

// Resource ID. Created from a string, hashed at compile-time if possible
struct ID {
	
	// Trivial default constructor
	constexpr ID() = default;
	
	// Hash string with FNV-1a
	explicit constexpr ID(string_view str): m_id(Basis) {
		
		for (auto ch: str) {
			m_id ^= ch;
			m_id *= Prime;
		}
		
	}
	
	// Zero-initializing constructor
	static constexpr auto make_default() -> ID { return ID(0u); }
	
	constexpr auto operator==(ID const&) const -> bool = default;
	constexpr auto operator!=(ID const&) const -> bool = default;
	constexpr auto operator+() const { return m_id; }
	
private:
	
	static constexpr auto Prime = 16777619u;
	static constexpr auto Basis = 2166136261u;
	
	uint m_id;
	
	// Construct with specified hash
	explicit constexpr ID(uint id): m_id(id) {}
	
};
static_assert(std::is_trivially_constructible_v<ID>);

// Guaranteed-constexpr string literal hash
consteval auto operator ""_id(char const* str, usize len) {
	
	return ID(string_view(str, len));
	
}

}

namespace std {

// Providing std::hash<ID>. The ID is already hashed, so this is an identity function
template<>
struct hash<minote::ID> {
	
	constexpr auto operator()(minote::ID const& id) const -> std::size_t { return +id; }
	
};

}

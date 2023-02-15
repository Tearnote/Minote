#pragma once

#include <string_view>
#include <type_traits>
#include "types.hpp"

namespace minote::util {

// Resource ID. Created from a string, hashed at compile-time if possible
class ID {

public:
	
	// Trivial default constructor
	constexpr ID() = default;
	
	// Hash string with FNV-1a
	explicit constexpr ID(std::string_view str): m_id(Basis) {
		
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

namespace id_literals {

// Guaranteed-constexpr string literal hash
consteval auto operator ""_id(char const* str, usize len) {
	
	return ID(std::string_view(str, len));
	
}

}

}

namespace std {

// Providing std::hash<ID>. The ID is already hashed, so this is an identity function
template<>
struct hash<minote::util::ID> {
	
	constexpr auto operator()(minote::util::ID const& id) const -> std::size_t { return +id; }
	
};

}

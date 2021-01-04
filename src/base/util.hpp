#pragma once

#include <concepts>
#include <cstddef>
#include "base/concepts.hpp"
#include "base/types.hpp"

namespace minote::base {

// Template replacement for the C offsetof() macro. Unfortunately, it cannot be constexpr within
// the current rules of the language.
// Example: offset_of(&Point::y)
// See: https://gist.github.com/graphitemaster/494f21190bb2c63c5516
template<typename T1, std::default_initializable T2>
inline auto offset_of(T1 T2::*member) -> size_t {
	static T2 obj;
	return reinterpret_cast<size_t>(&(obj.*member)) - reinterpret_cast<size_t>(&obj);
}

// Align a size to a given boundary.
constexpr auto alignSize(base::size_t size, base::size_t boundary) -> base::size_t {
	if (boundary == 0) return size;
	return (size + boundary - 1) & ~(boundary - 1);
}

namespace literals {

// Conversion of scoped enum to the underlying type, using the unary + operator
template<enum_type T>
constexpr auto operator+(T e) {
	return static_cast<std::underlying_type_t<T>>(e);
}

// size_t integer literal
consteval auto operator ""_zu(unsigned long long val) -> size_t {
	return static_cast<size_t>(val);
}

}

}

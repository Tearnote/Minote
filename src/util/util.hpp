#pragma once

#include <type_traits>
#include <concepts>
#include "types.hpp"
#include "stx/concepts.hpp"

namespace minote::util {

// Align a size to a given power-of-two boundary.
constexpr auto alignPOT(usize size, usize boundary) -> usize {
	
	if (boundary == 0) return size;
	return (size + boundary - 1) & ~(boundary - 1);
	
}

// Get the smallest power-of-two larger than the given number
constexpr auto nextPOT(uint n) -> uint {
	
	n -= 1;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n += 1;
	return n;
	
}

namespace enum_operators {

// Conversion of scoped enum to the underlying type, using the unary + operator
template<stx::enum_type T>
constexpr auto operator+(T e) { return std::underlying_type_t<T>(e); }

}

namespace storage_literals {

// Storage space literals
consteval auto operator ""_kb(unsigned long long val) { return val * 1024; }
consteval auto operator ""_mb(unsigned long long val) { return val * 1024 * 1024; }
consteval auto operator ""_gb(unsigned long long val) { return val * 1024 * 1024 * 1024; }

}

}

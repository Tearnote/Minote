#pragma once

#include <type_traits>
#include <ranges>
#include "util/concepts.hpp"
#include "util/types.hpp"

namespace minote {

// Selective imports of ranges library

using std::ranges::views::iota;
using std::ranges::views::reverse;

// Template replacement for the C offsetof() macro. Unfortunately, it cannot be constexpr within
// the current rules of the language.
// Example: offset_of(&Point::y)
// See: https://gist.github.com/graphitemaster/494f21190bb2c63c5516
template<typename T1, default_initializable T2>
inline auto offset_of(T1 T2::*member) -> usize {
	
	static auto obj = T2();
	return usize(&(obj.*member)) - usize(&obj);
	
}

// Align a size to a given power-of-two boundary.
constexpr auto alignPOT(usize size, usize boundary) -> usize {
	
	if (boundary == 0) return size;
	return (size + boundary - 1) & ~(boundary - 1);
	
}

constexpr auto nextPOT(u32 n) -> u32 {
	
	n -= 1;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n += 1;
	return n;
	
}

// Execute n times.
template<invocable F>
constexpr void repeat(usize times, F func) {
	
	for (auto i = usize(0); i < times; i += 1)
		func();
	
}

// defer pseudo-keyword for executing code at scope exit
// https://stackoverflow.com/a/42060129
#ifndef defer
struct defer_dummy {};
template <class F> struct deferrer { F f; ~deferrer() { f(); } };
template <class F> deferrer<F> operator*(defer_dummy, F f) { return {f}; }
#define DEFER_(LINE) zz_defer##LINE
#define DEFER(LINE) DEFER_(LINE)
#define defer auto DEFER(__LINE__) = defer_dummy{} *[&]()
#endif

// Conversion of scoped enum to the underlying type, using the unary + operator
template<enum_type T>
constexpr auto operator+(T e) { return std::underlying_type_t<T>(e); }

// usize integer literal
consteval auto operator ""_zu(unsigned long long val) -> usize { return val; }

// Storage space literals
consteval auto operator ""_kb(unsigned long long val) { return val * 1024; }
consteval auto operator ""_mb(unsigned long long val) { return val * 1024 * 1024; }
consteval auto operator ""_gb(unsigned long long val) { return val * 1024 * 1024 * 1024; }

}

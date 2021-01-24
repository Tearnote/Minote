#pragma once

#include <algorithm>
#include <concepts>
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
constexpr auto alignSize(size_t size, size_t boundary) -> size_t {
	if (boundary == 0) return size;
	return (size + boundary - 1) & ~(boundary - 1);
}

// Execute n times.
template<typename F>
	requires std::invocable<F>
void repeat(size_t times, F func) {
	for (size_t i = 0; i < times; i += 1)
		func();
}

// Numeric range, from start (inclusive) to end (exclusive), with optional step.
template<arithmetic T>
constexpr auto nrange(T start, T end, T step = 1) {
	return std::ranges::iota_view{static_cast<T>(0), (end - start + step - static_cast<T>(1)) / step} |
		std::views::transform([=](T n) { return n * step + start; });
}

// Numeric range, from start (inclusive) to end (inclusive), with optional step.
template<arithmetic T>
constexpr auto nrange_inc(T start, T end, T step = 1) {
	return std::ranges::iota_view{static_cast<T>(0), (end - start + step) / step} |
		std::views::transform([=](T n) { return n * step + start; });
}

// Reverse numeric range, from start (inclusive) to end (exclusive), with optional step.
template<arithmetic T>
constexpr auto rnrange(T start, T end, T step = 1) {
	return std::ranges::iota_view{static_cast<T>(0), (start - end + step - static_cast<T>(1)) / step} |
		std::views::transform([=](T n) { return start - n * step; });
}

// Reverse numeric range, from start (inclusive) to end (inclusive), with optional step.
template<arithmetic T>
constexpr auto rnrange_inc(T start, T end, T step = 1) {
	return std::ranges::iota_view{static_cast<T>(0), (start - end + step) / step} |
		std::views::transform([=](T n) { return start - n * step; });
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

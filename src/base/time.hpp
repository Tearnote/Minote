#pragma once

#include <concepts>
#include <chrono>
#include "base/concepts.hpp"
#include "base/types.hpp"

namespace minote::base {

// Main timestamp/duration type
using nsec = std::chrono::nanoseconds;

// Create nsec from a count of seconds
constexpr auto seconds(arithmetic auto val) {
	return nsec{static_cast<nsec::rep>(val * 1'000'000'000)};
}

// Create nsec from a count of milliseconds
constexpr auto milliseconds(arithmetic auto val) {
	return nsec{static_cast<nsec::rep>(val * 1'000'000)};
}

// Create nsec from second/millisecond literal
constexpr auto operator""_s(unsigned long long val) { return seconds(val); }
constexpr auto operator""_s(long double val) { return seconds(val); }
constexpr auto operator""_ms(unsigned long long val) { return milliseconds(val); }
constexpr auto operator""_ms(long double val) { return milliseconds(val); }

// If you use nsec's operators with floating-point numbers, the resulting value will
// be represented with a temporary higher precision type. use round() on the result to bring
// it back to nsec.
template<typename Rep, typename Period>
constexpr auto round(std::chrono::duration<Rep, Period> val) {
	return std::chrono::round<nsec>(val);
}

// Compute (left / right) with floating-point instead of integer division.
template<std::floating_point T = f32>
constexpr auto ratio(nsec const left, nsec const right) {
	return static_cast<T>(
		static_cast<double>(left.count()) / static_cast<double>(right.count())
	);
}

}

#pragma once

#include <concepts>
#include "types.hpp"
#include "stx/concepts.hpp"

namespace minote::stx {

// Main timestamp/duration type. Has enough resolution to largely ignore
// rounding error, and wraps after >100 years.
using nsec = int64;

// Create nsec from a count of seconds
template<stx::arithmetic T>
constexpr auto seconds(T val) -> nsec { return val * 1'000'000'000LL; }

// Create nsec from a count of milliseconds
template<stx::arithmetic T>
constexpr auto milliseconds(T val) { return val * 1'000'000LL; }

// Get an accurate floating-point ratio between two nsecs
template<std::floating_point T = float>
constexpr auto ratio(nsec left, nsec right) -> T { return double(left) / double(right); }

namespace time_literals {

// Create nsec from second/millisecond literals

constexpr auto operator""_s(unsigned long long val) { return seconds(val); }
constexpr auto operator""_s(long double val) { return seconds(val); }
constexpr auto operator""_ms(unsigned long long val) { return milliseconds(val); }
constexpr auto operator""_ms(long double val) { return milliseconds(val); }

}

}

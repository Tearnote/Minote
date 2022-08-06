#pragma once

#include "util/concepts.hpp"
#include "util/types.hpp"

namespace minote {

// Main timestamp/duration type. Has enough resolution to largely ignore
// rounding error, and wraps after >100 years.
using nsec = int64;

// Create nsec from a count of seconds
template<arithmetic T>
constexpr auto seconds(T val) -> nsec { return val * 1'000'000'000LL; }

// Create nsec from a count of milliseconds
template<arithmetic T>
constexpr auto milliseconds(T val) { return val * 1'000'000LL; }

// Get an accurate floating-point ratio between two nsecs
template<floating_point T = float>
constexpr auto ratio(nsec left, nsec right) -> T { return double(left) / double(right); }

// Create nsec from second/millisecond literals

constexpr auto operator""_s(unsigned long long val) { return seconds(val); }
constexpr auto operator""_s(long double val) { return seconds(val); }
constexpr auto operator""_ms(unsigned long long val) { return milliseconds(val); }
constexpr auto operator""_ms(long double val) { return milliseconds(val); }

}

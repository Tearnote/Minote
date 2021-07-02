#pragma once

#include "base/concepts.hpp"
#include "base/types.hpp"

namespace minote::base {

// Main timestamp/duration type. Has enough resolution to largely ignore
// rounding error, and wraps after >100 years.
using nsec = i64;

// Create nsec from a count of seconds
template<arithmetic T>
constexpr auto seconds(T val) -> nsec { return val * 1'000'000'000; }

// Create nsec from a count of milliseconds
template<arithmetic T>
constexpr auto milliseconds(T val) { return val * 1'000'000; }

// Get an accurate floating-point ratio between two nsecs
template<floating_point T = f32>
constexpr auto ratio(nsec left, nsec right) -> T { return f64(left) / f64(right); }

namespace literals {

// Create nsec from second/millisecond literal
constexpr auto operator""_s(unsigned long long val) { return seconds(val); }
constexpr auto operator""_s(long double val) { return seconds(val); }
constexpr auto operator""_ms(unsigned long long val) { return milliseconds(val); }
constexpr auto operator""_ms(long double val) { return milliseconds(val); }

}

}

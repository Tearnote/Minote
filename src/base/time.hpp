// Minote - base/time.hpp
// Types and utilities for measuring time

#pragma once

#include "base/math.hpp"
#include "base/util.hpp"

namespace minote {

// Count of nanoseconds, the main type used for timekeeping. The upper limit
// is about 290 years.
using nsec = i64;

// Format string for printing nsec values
#define PRInsec PRId64

// Create an nsec value out of a number of seconds.
template <Arithmetic T>
constexpr auto seconds(T const t) -> nsec {
	return t * 1'000'000'000LL;
}

// Create an nsec value out of a number of milliseconds.
template <Arithmetic T>
constexpr auto milliseconds(T const t) -> nsec {
	return t * 1'000'000LL;
}

// Sleep the thread for the specific duration. Keep in mind that on Windows this
// will be at least 1ms and might have strong jitter.
// This function is thread-safe.
void sleepFor(nsec duration);

}

// Minote - base/time.hpp
// Types and utilities for measuring time

#pragma once

#include <ctime>
#include "base/util.hpp"

namespace minote {

// Importing standard time functions and types
using std::time;
using std::localtime;
using std::time_t;
using std::tm;

// Count of nanoseconds, the main type used for timekeeping. The upper limit
// is about 290 years.
using nsec = i64;

// Create an nsec value out of a number of seconds.
template <arithmetic T>
constexpr auto seconds(T const t) -> nsec {
	return t * 1'000'000'000LL;
}

// Create an nsec value out of a number of milliseconds.
template <arithmetic T>
constexpr auto milliseconds(T const t) -> nsec {
	return t * 1'000'000LL;
}

}

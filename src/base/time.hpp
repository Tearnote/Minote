/**
 * Types and utilities for measuring time
 * @file
 */

#pragma once

#include <cstdint>
#include "base/util.hpp"

namespace minote {

/**
 * Count of nanoseconds. The main type used for timekeeping. The upper limit
 * is about 290 years.
 */
using nsec = std::int64_t;

/// Format string for printing nsec values
#define PRInsec PRId64

/**
 * Create an nsec value out of a number of seconds.
 * @param sec Number of seconds
 * @return Number of nanoseconds
 */
template <Arithmetic T>
constexpr auto seconds(T const t) -> nsec {
	return t * 1'000'000'000LL;
}

/**
 * Create an nsec value out of a number of seconds.
 * @param sec Number of seconds
 * @return Number of nanoseconds
 */
template <Arithmetic T>
constexpr auto milliseconds(T const t) -> nsec {
	return t * 1'000'000LL;
}

/**
 * Sleep the thread for the specific duration. Keep in mind that on Windows this
 * will be at least 1ms and might have strong jitter.
 * @param duration Requested sleep duration
 * @remark This function is thread-safe.
 */
void sleepFor(nsec duration);

}

/**
 * Types and utilities for measuring time
 * @file
 */

#ifndef MINOTE_TIME_H
#define MINOTE_TIME_H

#include <inttypes.h>
#include <stdint.h>

/**
 * Count of nanoseconds. The main type used for timekeeping. The upper limit
 * is about 290 years.
 */
#define nsec int64_t

/// Format string for printing nsec values
#define PRInsec PRId64

/**
 * Generic macro for converting seconds to nanoseconds
 * @param sec Number of seconds
 * @return Number of nanoseconds in ::nsec
 */
#define secToNsec(sec) \
    static_cast<nsec>((sec) * (nsec)1000000000)

/**
 * Return the time passed since systemInit().
 * @return Number of nanoseconds since system initialization
 * @remark This function is thread-safe.
 */
nsec getTime(void);

/**
 * Sleep the thread for the specific duration. Keep in mind that on Windows this
 * will be at least 1ms and might have strong jitter. Must be called after
 * systemInit().
 * @param duration Requested sleep duration
 * @remark This function is thread-safe.
 */
void sleepFor(nsec duration);

#endif //MINOTE_TIME_H

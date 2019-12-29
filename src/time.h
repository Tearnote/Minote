/**
 * Types and utilities for measuring time
 * @file
 */

#ifndef MINOTE_TIME_H
#define MINOTE_TIME_H

#include <inttypes.h>

/**
 * Count of nanoseconds. The main type used for timekeeping. The upper limit
 * is about 290 years.
 */
#define nsec int64_t

/// Format string for printing nsec values
#define PRInsec PRId64

/// Generic macro for converting seconds to nanoseconds
#define secToNsec(SEC) ((SEC) * (nsec)1000000000)

/**
 * Return the time passed since systemInit().
 * @return Number of nanoseconds since system initialization
 * @remark This function is thread-safe.
 */
nsec getTime(void);

/**
 * Do nothing until the specified time is reached. This executes busywait, so
 * use this for sleeping only if there is no better alternative.
 * @param until Target timestamp
 * @remark This function is thread-safe.
 */
void sleepUntil(nsec until);

#endif //MINOTE_TIME_H

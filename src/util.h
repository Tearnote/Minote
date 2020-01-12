/**
 * Useful functions and macros to compliment the standard library
 * @file
 */

#ifndef MINOTE_UTIL_H
#define MINOTE_UTIL_H

#include <stddef.h>

/**
 * A better replacement for NULL.
 * @see https://gustedt.wordpress.com/2010/11/07/dont-use-null/
 */
#define null 0

/**
 * A better replacement for M_PI.
 * @see https://tauday.com/
 */
#define M_TAU 6.28318530717958647693

/**
 * Macro to convert degrees to radians.
 * @param x Angle in degrees
 * @return Angle in radians
 */
#define radf(x) \
    ((x) * M_TAU / 360.0)

/**
 * Error-checking wrapper for calloc(). Clears memory to 0 and terminates
 * execution on error.
 * @param bytes Number of bytes to allocate, at least 1
 * @return Pointer to allocated memory
 */
void* alloc(size_t bytes);

/**
 * Error-checking wrapper for realloc(). Terminates execution on error, but
 * if the buffer is grown the additional space is not cleared to 0 - this should
 * be done manually to keep all data in a defined state.
 * @param buffer Pointer to previously allocated memory. Becomes undefined after
 * this call
 * @param newSize Target size to resize #buffer to, in bytes
 * @return Pointer to the resized buffer. The old value of @a buffer should be
 * immediately overwritten by this pointer
 */
void* ralloc(void* buffer, size_t newSize);

#endif //MINOTE_UTIL_H

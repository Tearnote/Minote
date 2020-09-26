/**
 * Useful functions and macros to compliment the standard library
 * @file
 */

#ifndef MINOTE_UTIL_H
#define MINOTE_UTIL_H

#include <stddef.h>
#include <stdlib.h> // Provide free()
#include <string.h>
#include "pcg/pcg_basic.h"

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

/// True modulo operation (as opposed to remainder, which is "%" in C.)
int mod(int num, int div);

/// PCG PRNG object. You can obtain an instance with rngCreate().
typedef pcg32_random_t Rng;

/**
 * Create a new ::Rng instance.
 * @param seed Initializer value. Using the same seed guarantees the same values
 * @return Newly created ::Rng. Must be destroyed with rngDestroy()
 */
Rng* rngCreate(uint64_t seed);

/**
 * Destroy a ::Rng instance. The destroyed object cannot be used anymore and
 * the pointer becomes invalid.
 * @param r The ::Rng object
 */
void rngDestroy(Rng* r);

/**
 * Return a random positive integer, up to a bound (exclusive). ::Rng state
 * is advanced by one step.
 * @param r The ::Rng object
 * @param bound The return value will be smaller than this argument
 * @return A random integer
 */
uint32_t rngInt(Rng* r, uint32_t bound);

/**
 * Return a random floating-point value between 0.0 (inclusive) and 1.0
 * (exclusive). ::Rng state is advanced by one step.
 * @param r The ::Rng object
 * @return A random floating-point number
 */
double rngFloat(Rng* r);

////////////////////////////////////////////////////////////////////////////////

#define countof(x) \
    ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

/**
 * Clear an array, setting all bytes to 0.
 * @param arr Array argument
 */
#define arrayClear(arr) \
    memset((arr), 0, sizeof((arr)))

/**
 * Copy the contents of one array into another array of the same or bigger size.
 * @param dst Destination of the copy
 * @param src Source of the copy
 */
#define arrayCopy(dst, src) \
    memcpy((dst), (src), sizeof((dst)))

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
 * @param newSize Target size to resize @a buffer to, in bytes
 * @return Pointer to the resized buffer. The old value of @a buffer should be
 * immediately overwritten by this pointer
 */
void* ralloc(void* buffer, size_t newSize);

#endif //MINOTE_UTIL_H

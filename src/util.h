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

/// Convenient rename of the PCG PRNG state
typedef pcg32_random_t rng;

/**
 * Initialize an ::rng with the provided seed.
 * @param[in,out] rngptr Pointer to a valid ::rng to initialize
 * @param seed uint64_t seed value
 */
#define srandom(rngptr, seed) \
        pcg32_srandom_r((rngptr), (seed), 'M'*'i'+'n'*'o'+'t'*'e')

/**
 * Pull a random number from 0 to an upper bound (exclusive).
 * @param rngptr[in,out] Pointer to an initialized ::rng
 * @param bound Exclusive upper bound
 * @return The random number
 */
#define random(rngptr, bound) \
        pcg32_boundedrand_r((rngptr), (bound))

/**
 * Pull a random floating-point number between 0.0 and 1.0 (exclusive).
 * @param rngptr[in,out] Pointer to an initialized ::rng
 * @return The random number
 */
#define frandom(rngptr) \
        ldexp(pcg32_random_r((rngptr)), -32)

#define countof(x) \
    ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

/**
 * Clear an array, setting all bytes to 0.
 * @param arr Array argument
 */
#define arrayClear(arr) \
    memset((arr), 0, sizeof((arr)))

/**
 * Clear a struct, setting all bytes to 0.
 * @param sct Struct instance argument
 */
#define structClear(sct) \
    memset(&(sct), 0, sizeof((sct)))

/**
 * Copy the contents of one array into another array of the same or bigger size.
 * @param dst Destination of the copy
 * @param src Source of the copy
 */
#define arrayCopy(dst, src) \
    memcpy((dst), (src), sizeof((dst)))

/**
 * Copy the contents of one struct instance into another instance of the same
 * struct.
 * @param dst Destination of the copy
 * @param src Source of the copy
 */
#define structCopy(dst, src) \
    memcpy(&(dst), &(src), sizeof((dst)))

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

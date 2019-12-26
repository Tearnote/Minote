/**
 * Useful functions and macros to compliment the standard library.
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
 * Error-checking wrapper for calloc(). Clears memory to 0 and terminates
 * execution on error.
 * @param bytes Number of bytes to allocate, at least 1
 * @return Pointer to allocated memory. Cannot be #null
 */
void* alloc(size_t bytes);

#endif //MINOTE_UTIL_H

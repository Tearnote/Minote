// Minote - util.h
// Various helper functions

#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "pcg/pcg_basic.h"

// Error-checking malloc wrappers. free() is fine as it is

void* allocate(size_t size);
void* reallocate(void* memory, size_t size);

// Convenient renamings for PCG PRNG
typedef pcg32_random_t rng;
#define srandom(rngptr, seed) pcg32_srandom_r((rngptr), (seed), 'M'*'i'+'n'*'o'+'t'*'e')
#define random(rngptr, bound) pcg32_boundedrand_r((rngptr), (bound))

// No C app is complete without its own assert
#ifdef _DEBUG
#define assert(cond) \
	(void) \
	((!!(cond)) || \
	(_assertFailed(#cond),0))
#else
#define assert(cond) ((void)0)
#endif

void _assertFailed(const char* cond);

#endif
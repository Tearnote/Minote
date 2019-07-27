// Minote - util.h
// Various helper functions

#ifndef UTIL_H
#define UTIL_H

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

#endif
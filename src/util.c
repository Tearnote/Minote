/**
 * Implementation of util.h
 * @file
 */

#include "util.h"

#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>

int mod(int num, int div)
{
	bool negative = num < 0;
	if (negative) num = -num;
	int result = num % div;
	if (negative) result = div - result;
	return result % div;
}

void* alloc(size_t bytes)
{
	assert(bytes);
	void* result = calloc(1, bytes);
	if (!result) {
		perror(u8"Could not allocate memory");
		exit(EXIT_FAILURE);
	}
	return result;
}

void* ralloc(void* buffer, size_t newSize)
{
	assert(buffer);
	assert(newSize);
	void* result = realloc(buffer, newSize);
	if (!result) {
		perror(u8"Could not reallocate memory");
		exit(EXIT_FAILURE);
	}
	return result;
}

////////////////////////////////////////////////////////////////////////////////

Rng* rngCreate(uint64_t seed)
{
	Rng* r = alloc(sizeof(*r));
	pcg32_srandom_r(r, seed, 'M'*'i'+'n'*'o'+'t'*'e');
	return r;
}

void rngDestroy(Rng* r)
{
	if (!r) return;
	free(r);
}

uint32_t rngInt(Rng* r, uint32_t bound)
{
	assert(r);
	assert(bound >= 1);
	return pcg32_boundedrand_r(r, bound);
}

double rngFloat(Rng* r)
{
	assert(r);
	return ldexp(pcg32_random_r(r), -32);
}

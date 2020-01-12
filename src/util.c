/**
 * Implementation of util.h
 * @file
 */

#include "util.h"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

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

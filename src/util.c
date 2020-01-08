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

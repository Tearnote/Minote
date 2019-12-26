#include "util.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

void* alloc(size_t bytes)
{
	assert(bytes);
	void* result = calloc(1, bytes);
	if (!result) {
		perror("Could not allocate memory");
		exit(EXIT_FAILURE);
	}
	return result;
}

// Minote - util.c

#include "util.h"

#include <stdlib.h>

#include "log.h"

void *allocate(size_t size)
{
	void *result = malloc(size);
	if (result == NULL) {
		logCrit("Failed to allocate %d bytes", size);
		exit(1);
	}
	return result;
}

void *reallocate(void *memory, size_t size)
{
	void *result = realloc(memory, size);
	if (result == NULL) {
		logCrit("Failed to reallocate %d bytes", size);
		exit(1);
	}
	return result;
}

void _assertFailed(const char *cond)
{
	logCrit("Assert failed: %s", cond);
	exit(1);
}
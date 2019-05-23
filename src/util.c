#include <util.h>

#include <stdlib.h>

#include "log.h"

void* allocate(int count, size_t size) {
	void* result = malloc(size * count);
	if(result == NULL) {
		logCrit("Failed to allocate %d bytes", size * count);
		exit(1);
	}
	return result;
}

void* reallocate(void* memory, int count, size_t size) {
	void* result = realloc(memory, size * count);
	if(result == NULL) {
		logCrit("Failed to reallocate %d bytes", size * count);
		exit(1);
	}
	return result;
}
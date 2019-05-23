#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>

// Error-checking malloc wrappers. free() is fine as it is
void* allocate(unsigned count, size_t size);
void* reallocate(void* memory, unsigned count, size_t size);

#endif
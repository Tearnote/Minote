/**
 * Implementation of darray.h
 * @file
 */

#include "darray.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

#define StartingSize 8

typedef struct darray {
	uint8_t* data; ///< Dynamically reallocated array for storing elements
	size_t elementSize; ///< Size of each element
	int count; ///< Number of elements currently in #data
	int capacity; ///< The maximum number of elements that can fit in #data
} darray;

darray* darrayCreate(size_t elementSize)
{
	assert(elementSize);
	darray* d = alloc(sizeof(*d));
	d->elementSize = elementSize;
	d->data = alloc(StartingSize * elementSize);
	d->capacity = StartingSize;
	return d;
}

void darrayDestroy(darray* d)
{
	assert(d);
	free(d->data);
	d->data = null;
	free(d);
	d = null;
}

void* darrayProduce(darray* d)
{
	assert(d);
	if (d->count == d->capacity) {
		size_t oldSize = d->capacity * d->elementSize;
		d->data = ralloc(d->data, oldSize * 2);
		memset(d->data + oldSize, 0, oldSize);
		d->capacity *= 2;
	}

	d->count += 1;
	return darrayGet(d, d->count);
}

void* darrayGet(darray* d, size_t index)
{
	assert(d);
	assert(index < d->count);
	return d->data + index * d->elementSize;
}

void darrayClear(darray* d)
{
	assert(d);
	d->count = 0;
}

size_t darraySize(darray* d)
{
	assert(d);
	return d->count;
}

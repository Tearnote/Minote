/**
 * Implementation of darray.h
 * @file
 */

#include "darray.hpp"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "util.hpp"

#define StartingSize 8

darray* darrayCreate(size_t elementSize)
{
	assert(elementSize);
	darray* d = static_cast<darray*>(alloc(sizeof(*d)));
	d->elementSize = elementSize;
	d->data = static_cast<uint8_t*>(alloc(StartingSize * elementSize));
	d->capacity = StartingSize;
	return d;
}

void darrayDestroy(darray* d)
{
	if (!d) return;
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
		d->data = static_cast<uint8_t*>(ralloc(d->data, oldSize * 2));
		memset(d->data + oldSize, 0, oldSize);
		d->capacity *= 2;
	}

	d->count += 1;
	return darrayGet(d, d->count - 1);
}

void darrayRemove(darray* d, size_t index)
{
	assert(d);
	assert(index < d->count);
	if (index < d->count - 1) {
		memmove(d->data + index * d->elementSize,
			d->data + (index + 1) * d->elementSize,
			(d->count - index - 1) * d->elementSize);
	}
	d->count -= 1;
}

void darrayRemoveSwap(darray* d, size_t index)
{
	assert(d);
	assert(index < d->count);
	if (index < d->count - 1) {
		memcpy(d->data + index * d->elementSize,
			d->data + (d->count - 1) * d->elementSize,
			d->elementSize);
	}
	d->count -= 1;
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

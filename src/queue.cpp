/**
 * Implementation of queue.h
 * @file
 */

#include "queue.hpp"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "util.hpp"

queue* queueCreate(size_t elementSize, size_t maxElements)
{
	assert(elementSize);
	assert(maxElements);
	assert(elementSize <= SIZE_MAX / (maxElements + 1)); // Overflow check
	queue* q = static_cast<queue*>(alloc(sizeof(*q)));
	// In the implementation at least 1 element needs to be free, to prevent
	// confusion between empty and full states
	q->capacity = maxElements + 1;
	q->elementSize = elementSize;
	q->data = static_cast<uint8_t*>(alloc(q->elementSize * q->capacity));
	return q;
}

void queueDestroy(queue* q)
{
	if (!q) return;
	free(q->data);
	q->data = null;
	free(q);
}

bool queueEnqueue(queue* q, void* element)
{
	assert(q);
	assert(element);
	if (queueIsFull(q)) return false;
	memcpy(q->data + (q->head * q->elementSize), element, q->elementSize);
	q->head = (q->head + 1) % q->capacity;
	return true;
}

bool queueDequeue(queue* q, void* element)
{
	assert(q);
	assert(element);
	if (queueIsEmpty(q)) return false;
	memcpy(element, q->data + (q->tail * q->elementSize), q->elementSize);
	q->tail = (q->tail + 1) % q->capacity;
	return true;
}

bool queuePeek(queue* q, void* element)
{
	assert(q);
	assert(element);
	if (queueIsEmpty(q)) return false;
	memcpy(element, q->data + (q->tail * q->elementSize), q->elementSize);
	return true;
}

bool queueIsEmpty(queue* q)
{
	assert(q);
	return q->head == q->tail;
}

bool queueIsFull(queue* q)
{
	assert(q);
	return (q->head + 1) % q->capacity == q->tail;
}

void queueClear(queue* q)
{
	assert(q);
	q->head = 0;
	q->tail = 0;
}

// Minote - queue.c

#include "queue.h"

#include <stdlib.h>

#include "util.h"

queue* createQueue(size_t itemSize) {
	queue* q = allocate(sizeof(queue));
	q->itemSize = itemSize;
	q->buffer = allocate(itemSize);
	q->allocated = 1;
	q->count = 0;
	return q;
}

void destroyQueue(queue* q) {
	free(q->buffer);
	free(q);
}

void* produceQueueItem(queue* q) {
	// Grow the buffer if there is no space left
	if(q->count == q->allocated) {
		q->buffer = reallocate(q->buffer, q->allocated * q->itemSize * 2);
		q->allocated *= 2;
	}
	q->count += 1;
	return getQueueItem(q, q->count - 1); // Simply return the first unused item and mark it as used
}

void* getQueueItem(queue* q, unsigned index) {
	return (char*)q->buffer + index*q->itemSize;
}

void clearQueue(queue* q) {
	q->count = 0;
}
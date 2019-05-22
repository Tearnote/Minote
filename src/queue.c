#include "queue.h"

#include <stdlib.h>

#include "log.h"

queue* createQueue(size_t itemSize) {
	queue* q = malloc(sizeof(queue));
	q->itemSize = itemSize;
	q->buffer = malloc(itemSize);
	q->allocated = 1;
	q->count = 0;
	return q;
}

void destroyQueue(queue* q) {
	free(q->buffer);
	free(q);
}

void* produceQueueItem(queue* q) {
	if(q->count == q->allocated) {
		q->buffer = realloc(q->buffer, q->allocated*2 * q->itemSize);
		q->allocated *= 2;
	}
	q->count += 1;
	return getQueueItem(q, q->count - 1);
}

void* getQueueItem(queue* q, unsigned index) {
	return (char*)q->buffer + index*q->itemSize;
}

void clearQueue(queue* q) {
	q->count = 0;
}
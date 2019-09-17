// Minote - queue.c

#include "queue.h"

#include <stdlib.h>

#include "util.h"

struct queue *createQueue(size_t itemSize)
{
	struct queue *q = allocate(sizeof(*q));
	q->itemSize = itemSize;
	q->buffer = allocate(itemSize);
	q->allocated = 1;
	q->count = 0;
	return q;
}

void destroyQueue(struct queue *q)
{
	free(q->buffer);
	free(q);
}

void *produceQueueItem(struct queue *q)
{
	// Grow the buffer if there is no space left
	if (q->count == q->allocated) {
		size_t oldSize = q->allocated * q->itemSize;
		q->buffer = reallocate(q->buffer, oldSize * 2);
		memset((char *)q->buffer + oldSize, 0, oldSize);
		q->allocated *= 2;
	}
	q->count += 1;
	// Simply return the first unused item and mark it as used
	return getQueueItem(q, q->count - 1);
}

#include "log.h"

void *getQueueItem(struct queue *q, int index)
{
	return (char *)q->buffer + index * q->itemSize;
}

void clearQueue(struct queue *q)
{
	q->count = 0;
}

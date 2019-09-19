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
		memset(q->buffer + oldSize, 0, oldSize);
		q->allocated *= 2;
	}
	q->count += 1;
	// Simply return the first unused item and mark it as used
	return getQueueItem(q, q->count - 1);
}

void *getQueueItem(struct queue *q, int index)
{
	return q->buffer + index * q->itemSize;
}

void clearQueue(struct queue *q)
{
	q->count = 0;
}

struct vqueue *createVqueue(void)
{
	struct vqueue *vq = allocate(sizeof(*vq));
	vq->buffer = allocate(1);
	vq->allocated = 1;
	return vq;
}

void destroyVqueue(struct vqueue *vq)
{
	free(vq->buffer);
	free(vq);
}

void *produceVqueueItem(struct vqueue *vq, size_t itemSize)
{
	while (vq->size + itemSize > vq->allocated) {
		vq->buffer = reallocate(vq->buffer, vq->allocated * 2);
		memset(vq->buffer + vq->allocated, 0, vq->allocated);
		vq->allocated *= 2;
	}

	void *result = vq->buffer + vq->size;
	vq->size += itemSize;
	return result;
}

void *getVqueueItem(struct vqueue *vq, size_t offset)
{
	return vq->buffer + offset;
}

void clearVqueue(vqueue *vq)
{
	vq->size = 0;
}
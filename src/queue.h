#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>

typedef struct {
	void* buffer;
	size_t itemSize;
	unsigned count;
	size_t allocated;
} queue;

queue* createQueue(size_t itemSize);
void destroyQueue(queue* q);
void* produceQueueItem(queue* q);
void* getQueueItem(queue* q, unsigned index);
void clearQueue(queue* q);

#endif
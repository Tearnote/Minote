// Minote - queue.h
// Data structure similar to a dynamic array, it's a single block of allocated memory
// Requesting more items than there is allocated memory will grow the buffer 2x
// Repeatedly filling it up and clearing with similar amounts of items are very fast operations
// However, a spike in size will cause a lot of wasted memory for the rest of its existence

#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>

typedef struct {
	void* buffer;
	size_t itemSize;
	unsigned count; // Number of items present
	unsigned allocated; // Number of items that can fit in the buffer
} queue;

queue* createQueue(size_t itemSize);
void destroyQueue(queue* q); // Does not need the queue to be empty
void* produceQueueItem(queue* q); // Do not free the returned item. Memory is uninitialized
void* getQueueItem(queue* q, unsigned index);
void clearQueue(queue* q);

#endif
// Minote - queue.h
// Data structure similar to a dynamic array, it's a single block of allocated
// memory. Requesting more items than there is allocated memory will grow
// the buffer 2x. Repeatedly filling it up and clearing with similar amounts
// of items are very fast operations. However, a spike in size will cause a lot
// of wasted memory for the rest of its existence

#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>

typedef struct queue {
	void *buffer;
	size_t itemSize;
	unsigned count; // Number of items present
	unsigned allocated; // Number of items that can fit in the buffer
} queue;

// Initialize an empty queue
queue *createQueue(size_t itemSize);

// Destroy a queue
// Does not need the queue to be empty
void destroyQueue(queue *q);

// Return a new, unused item
// Do not free the returned item
// The memory is uninitialized
void *produceQueueItem(queue *q);

void *getQueueItem(queue *q, int index);
void clearQueue(queue *q);

#endif // QUEUE_H

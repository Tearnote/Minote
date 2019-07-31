// Minote - fifo.h
// A generic FIFO
// Elements need to be allocated and freed manually
// Empty the FIFO before destroying

#ifndef FIFO_H
#define FIFO_H

#include <stdbool.h>

typedef struct fifoItem {
	void* data;
	struct fifoItem* next;
} fifoItem;

typedef struct {
	fifoItem* first;
	fifoItem* last;
} fifo;

fifo* createFifo(void);
void destroyFifo(fifo* f);
void enqueueFifo(fifo* f, void* data);
void* dequeueFifo(fifo* f);
bool isFifoEmpty(fifo* f);

#endif
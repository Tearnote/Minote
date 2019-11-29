// Minote - types/fifo.h
// A generic FIFO
// Elements need to be allocated and freed manually
// Empty the FIFO before destroying

#ifndef TYPES_FIFO_H
#define TYPES_FIFO_H

#include <stdbool.h>

struct fifoItem {
	void *data;
	struct fifoItem *next;
};

typedef struct fifo {
	struct fifoItem *first;
	struct fifoItem *last;
} fifo;

fifo *createFifo(void);
void destroyFifo(fifo *f);
void enqueueFifo(fifo *f, void *data);
void *dequeueFifo(fifo *f);
bool isFifoEmpty(fifo *f);

#endif // TYPES_FIFO_H

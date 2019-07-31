// Minote - fifo.c

#include "fifo.h"

#include <stdlib.h>
#include <stdbool.h>

#include "util.h"
#include "log.h"

fifo* createFifo(void) {
	fifo* f = allocate(sizeof(fifo));
	f->first = NULL;
	f->last = NULL;
	return f;
}

void destroyFifo(fifo* f) {
	if(!isFifoEmpty(f)) logWarn("Destroying a nonempty FIFO");
	free(f);
}

void enqueueFifo(fifo* f, void* data) {
	fifoItem* item = allocate(sizeof(fifoItem));
	item->data = data;
	item->next = NULL;
	if(f->last) f->last->next = item;
	f->last = item;
	if(isFifoEmpty(f)) f->first = item;
}

void* dequeueFifo(fifo* f) {
	if(isFifoEmpty(f)) return NULL;
	fifoItem* item = f->first;
	f->first = item->next;
	if(!item->next) f->last = NULL;
	void* data = item->data;
	free(item);
	return data;
}

bool isFifoEmpty(fifo* f) {
	return !f->first;
}
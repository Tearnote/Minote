#include "fifo.h"

#include <stdlib.h>
#include <stdbool.h>

#include "util.h"
#include "log.h"

fifo* createFifo(void) {
	fifo* f = allocate(1, sizeof(fifo));
	f->first = NULL;
	f->last = NULL;
	return f;
}

void destroyFifo(fifo* f) {
	if(f->first) logWarn("Destroying a nonempty FIFO");
	free(f);
}

void enqueueFifo(fifo* f, void* data) {
	fifoItem* item = allocate(1, sizeof(fifoItem));
	item->data = data;
	item->prev = f->last;
	item->next = NULL;
	if(f->last) f->last->next = item;
	f->last = item;
	if(!f->first) f->first = item;
}

void* dequeueFifo(fifo* f) {
	if(!f->first) return NULL;
	fifoItem* item = f->first;
	if(item->next) item->next->prev = NULL;
	f->first = item->next;
	if(!item->next) f->last = NULL;
	void* data = item->data;
	free(item);
	return data;
}

bool isFifoEmpty(fifo* f) {
	return !!f->first;
}
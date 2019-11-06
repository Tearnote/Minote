// Minote - fifo.c

#include "fifo.h"

#include <stdlib.h>
#include <stdbool.h>

#include "util/util.h"
#include "util/log.h"

struct fifo *createFifo(void)
{
	struct fifo *f = allocate(sizeof(*f));
	f->first = NULL;
	f->last = NULL;
	return f;
}

void destroyFifo(struct fifo *f)
{
	if (!isFifoEmpty(f))
		logWarn("Destroying a nonempty FIFO");
	free(f);
}

void enqueueFifo(struct fifo *f, void *data)
{
	struct fifoItem *item = allocate(sizeof(*item));
	item->data = data;
	item->next = NULL;
	if (f->last)
		f->last->next = item;
	f->last = item;
	if (isFifoEmpty(f))
		f->first = item;
}

void *dequeueFifo(struct fifo *f)
{
	if (isFifoEmpty(f))
		return NULL;
	struct fifoItem *item = f->first;
	f->first = item->next;
	if (!item->next)
		f->last = NULL;
	void *data = item->data;
	free(item);
	return data;
}

bool isFifoEmpty(struct fifo *f)
{
	return !f->first;
}

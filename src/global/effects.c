// Minote - global/effects.c

#include "global/effects.h"

#include <stdlib.h>

#include "types/fifo.h"
#include "util/thread.h"

fifo *effects = NULL;
mutex effectsMutex = newMutex;

void initEffects(void)
{
	if (effects)
		return;
	effects = createFifo();
}

void cleanupEffects(void)
{
	if (!effects)
		return;
	// The fifo might not be empty so we clear it
	struct effect *e;
	while ((e = dequeueEffect())) {
		if (e->data)
			free(e->data);
		free(e);
	}
	destroyFifo(effects);
	effects = NULL;
}

void enqueueEffect(struct effect *e)
{
	syncFifoEnqueue(effects, e, &effectsMutex);
}

struct effect *dequeueEffect(void)
{
	return syncFifoDequeue(effects, &effectsMutex);
}
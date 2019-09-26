// Minote - effects.c

#include "effects.h"

#include <stdlib.h>

#include "fifo.h"
#include "thread.h"

fifo *effects = NULL;
mutex effectsMutex = newMutex;

void initEffects(void)
{
	effects = createFifo();
}

void cleanupEffects(void)
{
	if (!effects)
		return;
	// The fifo might not be empty so we clear it
	struct effect *e;
	while ((e = dequeueEffect()))
		free(e);
	destroyFifo(effects);
	effects = NULL;
}
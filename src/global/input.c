// Minote - global/input.c

#include "global/input.h"

#include "types/fifo.h"
#include "util/util.h"
#include "util/thread.h"

fifo *inputs = NULL;
mutex inputMutex = newMutex;

void initInput(void)
{
	inputs = createFifo();
}

void cleanupInput(void)
{
	if (!inputs)
		return;
	// The fifo might not be empty so we clear it
	struct input *i;
	while ((i = dequeueInput()))
		free(i);
	destroyFifo(inputs);
	inputs = NULL;
}

void enqueueInput(struct input *i)
{
	syncFifoEnqueue(inputs, i, &inputMutex);
}

struct input *dequeueInput(void)
{
	return syncFifoDequeue(inputs, &inputMutex);
}

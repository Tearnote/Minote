#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>

#include "thread.h"
#include "fifo.h"

typedef enum {
	InputLeft, InputRight,
	InputHard, InputSonic,
	InputRotCW, InputRotCCW,
	InputBack, InputAccept,
	InputSize
} inputType;

typedef enum {
	actionPressed,
	actionReleased,
	actionSize
} inputAction;

typedef struct {
	inputType type;
	inputAction action;
} input;

extern fifo* inputs;
extern mutex inputMutex;

void initInput(void);
void cleanupInput(void);
void updateInput(void);
void sleepInput(void);
#define enqueueInput(i) syncFifoEnqueue(inputs, (i), &inputMutex)
#define dequeueInput() syncFifoDequeue(inputs, &inputMutex)

#endif
#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>

#include "thread.h"
#include "fifo.h"
#include "timer.h"

typedef enum {
	InputNone,
	InputLeft, InputRight,
	InputHard, InputSonic,
	InputRotCW, InputRotCCW,
	InputBack, InputAccept,
	InputSize
} inputType;

typedef enum {
	ActionNone,
	ActionPressed,
	ActionReleased,
	ActionSize
} inputAction;

typedef struct {
	inputType type;
	inputAction action;
	nsec timestamp;
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
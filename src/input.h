// Minote - input.h
// Polls devices for inputs, converts to generic controls and puts them in a FIFO

#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>

#include "thread.h"
#include "fifo.h"

// Generic list of inputs used by the game
enum inputType {
	InputNone,
	InputLeft, InputRight, InputUp, InputDown,
	InputButton1, InputButton2, InputButton3, InputButton4,
	InputStart, InputQuit,
	InputSize
};

enum inputAction {
	ActionNone,
	ActionPressed,
	ActionReleased,
	ActionSize
};

struct input {
	enum inputType type;
	enum inputAction action;
	//nsec timestamp;
};

extern fifo *inputs; //SYNC inputMutex enqueueInput dequeueInput
extern mutex inputMutex;

void initInput(void);
void cleanupInput(void);
void updateInput(void);
void sleepInput(void);

#define enqueueInput(i) \
        syncFifoEnqueue(inputs, (i), &inputMutex)
#define dequeueInput() \
        syncFifoDequeue(inputs, &inputMutex)

#endif // INPUT_H

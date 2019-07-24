#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>

#include "thread.h"

typedef enum {
	InputNone,
	InputLeft, InputRight,
	InputSoft, InputSonic,
	InputRotCW, InputRotCCW,
	InputBack, InputAccept,
	InputSize
} inputType;

extern bool inputs[InputSize]; //SYNC inputMutex
extern mutex inputMutex;

void initInput(void);
void cleanupInput(void);
void updateInput(void);
void sleepInput(void);

#endif
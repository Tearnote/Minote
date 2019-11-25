// Minote - logic/logic.h
// A thread that handles and advances state

#ifndef LOGIC_LOGIC_H
#define LOGIC_LOGIC_H

#include "util/thread.h"

#define DEFAULT_FREQUENCY 59.84

extern atomic double logicFrequency; // in Hz

void spawnLogic(void);
void awaitLogic(void);

#endif //LOGIC_LOGIC_H

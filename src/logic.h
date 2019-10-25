// Minote - logic.h
// A thread that handles and advances state
// Currently only gameplay state exists

#ifndef LOGIC_H
#define LOGIC_H

#include "thread.h"

#define DEFAULT_FREQUENCY 59.84

extern atomic double logicFrequency; // in Hz

void *logicThread(void *param);
extern thread logicThreadID;
#define spawnLogic() \
        spawnThread(&logicThreadID, logicThread, NULL, "logicThread")
#define awaitLogic() \
        awaitThread(logicThreadID)

#endif // LOGIC_H
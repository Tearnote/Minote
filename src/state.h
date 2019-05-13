#ifndef STATE_H
#define STATE_H

#include <stdbool.h>

#include "thread.h"

extern bool running;
extern mutex runningMutex;
#define isRunning() syncBoolRead(&running, &runningMutex)
#define setRunning(x) syncBoolWrite(&running, (x), &runningMutex)

#endif
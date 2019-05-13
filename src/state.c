#include "state.h"

#include <stdbool.h>

#include "thread.h"

bool running = true;
mutex runningMutex = newMutex;
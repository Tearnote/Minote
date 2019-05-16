#ifndef LOGIC_H
#define LOGIC_H

#include "thread.h"

void* logicThread(void* param);
extern thread logicThreadID;
#define spawnLogic() spawnThread(&logicThreadID, logicThread, NULL, "logicThread")
#define awaitLogic() awaitThread(logicThreadID)

#endif
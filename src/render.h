#ifndef RENDER_H
#define RENDER_H

#include <stdbool.h>

#include "thread.h"

void* rendererThread(void* param);
extern thread rendererThreadID;
#define spawnRenderer() spawnThread(&rendererThreadID, rendererThread, NULL, "rendererThread")
#define awaitRenderer() awaitThread(rendererThreadID)

#endif
#include "logic.h"

#include <stdbool.h>

#include "thread.h"
#include "state.h"
#include "timer.h"
#include "log.h"
#include "input.h"

thread logicThreadID = 0;

#define LOGIC_FREQUENCY 1000 // in Hz
#define TIME_PER_UPDATE (SEC / LOGIC_FREQUENCY)
static nsec updateTime = 0; // This update's timestamp

static void updateLogic(void) {
	updateTime = getTime();
	
	updateGameplay(updateTime);
}

static void sleepLogic(void) {
	nsec timePassed = getTime() - updateTime;
	if(timePassed < TIME_PER_UPDATE) // Only bother sleeping if we're ahead of the target
		sleep(TIME_PER_UPDATE - timePassed);
}

void* logicThread(void* param) {
	(void)param;
	
	initGameplay();
	
	while(isRunning()) {
		updateLogic();
		sleepLogic();
	}
	
	return NULL;
}
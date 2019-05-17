#include "logic.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include <stdint.h>
#include <time.h>

#include "thread.h"
#include "state.h"
#include "timer.h"
#include "log.h"

#define INPUT_FREQUENCY 1000 // in Hz
#define TIME_PER_UPDATE (SEC / INPUT_FREQUENCY)
static nsec lastUpdateTime = 0;

thread logicThreadID = 0;

void updateLogic() {
	lastUpdateTime = getTime();
}

void sleepLogic() {
	nsec timePassed = getTime() - lastUpdateTime;
	if(timePassed < TIME_PER_UPDATE) // Only bother sleeping if we're ahead of the target
		sleep(TIME_PER_UPDATE - timePassed);
}

void* logicThread(void* param) {
	(void)param;
	
	while(isRunning()) {
		updateLogic();
		sleepLogic();
	}
	
	return NULL;
}
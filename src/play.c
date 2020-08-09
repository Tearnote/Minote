/**
 * Implementation of play.h
 * @file
 */

#include "play.h"

#include <assert.h>
#include "window.h"
#include "mapper.h"
#include "darray.h"
#include "util.h"
#include "time.h"
//#include "pure.h"
#include "mrs.h"
#include "log.h"

/// Timestamp of the next game logic update
static nsec nextUpdate = 0;

/// List of collectedInputs for the next logic frame to process
static darray* collectedInputs = null;

static bool initialized = false;

void playInit(void)
{
	if (initialized) return;

	collectedInputs = darrayCreate(sizeof(Input));
//	nextUpdate = getTime() + PureUpdateTick;
	nextUpdate = getTime() + MrsUpdateTick;
//	pureInit();
	mrsInit();

	initialized = true;
	logDebug(applog, u8"Play layer initialized");
}

void playCleanup(void)
{
	if (!initialized) return;

//	pureCleanup();
	mrsCleanup();
	if (collectedInputs) {
		darrayDestroy(collectedInputs);
		collectedInputs = null;
	}

	initialized = false;
	logDebug(applog, u8"Play layer cleaned up");
}

void playUpdate(void)
{
	assert(initialized);

	// Update as many times as we need to catch up
	while (nextUpdate <= getTime()) {
		Input i;
		while (mapperPeek(&i)) { // Exhaust all collectedInputs...
			if (i.timestamp <= nextUpdate)
				mapperDequeue(darrayProduce(collectedInputs));
			else
				break; // Or abort if we encounter an input from the future

			// Interpret quit events here for now
			if (i.type == InputQuit && i.state)
				windowClose();
		}

//		pureAdvance(collectedInputs);
		mrsAdvance(collectedInputs);
		darrayClear(collectedInputs);
//		nextUpdate += PureUpdateTick;
		nextUpdate += MrsUpdateTick;
	}
}

void playDraw(void)
{
	assert(initialized);
//	pureDraw();
	mrsDraw();
}

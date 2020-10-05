/**
 * Implementation of play.h
 * @file
 */

#include "play.hpp"

#include <assert.h>
#include "sys/window.hpp"
#include "mapper.hpp"
#include "base/varray.hpp"
#include "base/util.hpp"
#include "mrs.hpp"
#include "base/log.hpp"

using minote::L;
using minote::varray;

/// Timestamp of the next game logic update
static nsec nextUpdate = 0;

/// List of collectedInputs for the next logic frame to process
static varray<Input, 64> collectedInputs{};

static bool initialized = false;

void playInit(void)
{
	if (initialized) return;

	nextUpdate = getTime() + MrsUpdateTick;
	mrsInit();

	initialized = true;
	L.debug("Play layer initialized");
}

void playCleanup(void)
{
	if (!initialized) return;

	mrsCleanup();

	initialized = false;
	L.debug("Play layer cleaned up");
}

void playUpdate(void)
{
	assert(initialized);

	// Update as many times as we need to catch up
	while (nextUpdate <= getTime()) {
		Input i;
		while (mapperPeek(&i)) { // Exhaust all collectedInputs...
			if (i.timestamp <= nextUpdate)
				mapperDequeue(collectedInputs.produce());
			else
				break; // Or abort if we encounter an input from the future

			// Interpret quit events here for now
			if (i.type == InputQuit && i.state)
				windowClose();
		}

		mrsAdvance(collectedInputs);
		collectedInputs.clear();
		nextUpdate += MrsUpdateTick;
	}
}

void playDraw(void)
{
	assert(initialized);
	mrsDraw();
}

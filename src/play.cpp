/**
 * Implementation of play.h
 * @file
 */

#include "play.hpp"

#include "sys/window.hpp"
#include "mapper.hpp"
#include "base/varray.hpp"
#include "base/util.hpp"
#include "mrs.hpp"
#include "base/log.hpp"

using namespace minote;

/// Timestamp of the next game logic update
static minote::nsec nextUpdate = 0;

/// List of collectedInputs for the next logic frame to process
static varray<Input, 64> collectedInputs{};

static bool initialized = false;

void playInit(void)
{
	if (initialized) return;

	nextUpdate = Window::getTime() + MrsUpdateTick;
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
	ASSERT(initialized);

	// Update as many times as we need to catch up
	while (nextUpdate <= Window::getTime()) {
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
	ASSERT(initialized);
	mrsDraw();
}

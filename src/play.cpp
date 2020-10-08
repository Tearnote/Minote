/**
 * Implementation of play.h
 * @file
 */

#include "play.hpp"

#include "sys/window.hpp"
#include "mapper.hpp"
#include "mrs.hpp"
#include "base/log.hpp"

using namespace minote;

/// Timestamp of the next game logic update
static minote::nsec nextUpdate = 0;

/// List of collectedInputs for the next logic frame to process
static varray<Action, 64> collectedInputs{};

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

void playUpdate(Window& window, Mapper& mapper)
{
	ASSERT(initialized);

	// Update as many times as we need to catch up
	while (nextUpdate <= Window::getTime()) {
		while (const auto actionOpt = mapper.peekAction()) { // Exhaust all actions...
			const auto action = actionOpt.value();

			if (action.timestamp <= nextUpdate) {
				// collectedInputs is the same size, so this should never be nullopt
				auto& nextAction = collectedInputs.produce().value();
				nextAction = action;
				mapper.dequeueAction();
			} else {
				break; // Or abort if we encounter an action from the future
			}

			// Interpret quit events here for now
			if (action.type == Action::Type::Back)
				window.requestClose();
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

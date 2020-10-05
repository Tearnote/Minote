/**
 * Implementation of time.h
 * @file
 */

#include "base/time.hpp"

#include <thread>
#include <chrono>
#include "base/util.hpp"

namespace minote {

void sleepFor(nsec duration)
{
	ASSERT(duration > 0);
	std::this_thread::sleep_for(std::chrono::nanoseconds(duration));
}

}

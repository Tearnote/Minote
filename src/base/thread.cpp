#include "base/thread.hpp"

namespace minote {

#include <thread>
#include <chrono>

void sleepFor(nsec const duration)
{
	if (duration <= 0) return;
	std::this_thread::sleep_for(std::chrono::nanoseconds(duration));
}

}

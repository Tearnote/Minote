#include "base/time.hpp"

#include <thread>
#include <chrono>

namespace minote {

void sleepFor(nsec const duration)
{
	ASSERT(duration > 0);
	std::this_thread::sleep_for(std::chrono::nanoseconds(duration));
}

}

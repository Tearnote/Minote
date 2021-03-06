#include "base/thread.hpp"

namespace minote {

#include <thread>

void sleepFor(nsec const duration) {
	if (duration.count() <= 0) return;
	std::this_thread::sleep_for(duration);
}

}

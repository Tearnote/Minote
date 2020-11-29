// Minote - base/thread.hpp
// Standard threading utilities

#pragma once

#include <atomic>
#include <thread>
#include <mutex>
#include "base/time.hpp"

namespace minote {

using thread = std::jthread; // Joins thread on destruction
using std::mutex;
using std::lock_guard;
using std::atomic;

// Sleep the thread for the specific duration. Keep in mind that on Windows this
// will be at least 1ms and might have strong jitter.
// This function is thread-safe.
void sleepFor(nsec duration);

}

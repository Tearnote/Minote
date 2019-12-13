// Minote - system.h

#ifndef MINOTE_SYSTEM_H
#define MINOTE_SYSTEM_H

#include <atomic>
#include <string_view>

// Represents an initialized graphics and input system
// Only up to one instance can exist at a time
// Can only be used from the main thread
class System {
public:
	System();
	~System();

	// Check for user events and dispatch windows' callbacks
	// Loop on this to keep the application responsive
	auto update() -> void;

	// Throw an exception that includes a system error code and message
	auto throwSystemError(std::string_view str) -> void;

private:
	static inline std::atomic<bool> exists{false};
};

#endif //MINOTE_SYSTEM_H

// Minote - main.cpp

#include "log.h"
#include "system.h"
#include "window.h"

auto main() -> int
{
	try {
#if NDEBUG
		Log::setLevel(Log::Info);
#else //NDEBUG
		Log::enable(Log::Console);
		Log::setLevel(Log::Debug);
#endif //NDEBUG
		Log::enable(Log::File);
		Log::info("Starting up Minote 0.0++");

		System system{};
		Window window{system, "Minote"};

		while (window.isOpen()) {
			system.update();
			while (auto i = window.popInput())
				Log::debug("Keypress ", i->key, "/", i->action, " at ", i->timestamp);
		}

		Log::info("Shutting down");
	}
	catch (std::exception& e) {
		Log::crit("Exception caught: ", e.what());
		Log::disable(Log::File);
		return 1;
	}
	return 0;
}

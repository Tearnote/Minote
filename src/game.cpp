// Minote - game.cpp

#include "game.h"
#include "log.h"
#include "renderer.h"

Game::Game(Inputs i)
		:window{i.w},
		 thread{&Game::run, this} { }

Game::~Game()
{
	thread.join();
}

auto Game::run() -> void
{
	try {
		Renderer renderer{window};

		while (window.isOpen()) {
			while (auto i = window.popInput()) {
				if (i->action != GLFW_PRESS)
					continue;
				Log::debug("Key pressed: ", i->key, " at ", i->timestamp);
				if (i->key == GLFW_KEY_ESCAPE)
					window.close();
			}

			renderer.render();
		}
	}
	catch (std::exception &e) {
		Log::crit("Exception caught: ", e.what());
		Log::disable(Log::File);
		window.close();
	}
}

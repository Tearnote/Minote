// Minote - game.cpp

#include "game.h"
#include "log.h"
#include "renderer.h"
#include "gameplay.h"

Game::Game(Inputs i)
		:window{i.window},
		 thread{&Game::run, this} { }

Game::~Game()
{
	thread.join();
}

auto Game::run() -> void
{
	try {
		Renderer renderer{window};
		states.add(std::make_unique<Gameplay>(*this));

		while (window.isOpen()) {
			while (auto i = window.popInput()) {
				if (i->action != GLFW_PRESS)
					continue;
				Log::debug("Key pressed: ", i->key, " at ", i->timestamp);
				if (i->key == GLFW_KEY_ESCAPE)
					window.close();
			}

			states.update();

			renderer.render();
		}
	}
	catch (std::exception &e) {
		Log::crit("Exception caught: ", e.what());
		Log::disable(Log::File);
		window.close();
	}
}

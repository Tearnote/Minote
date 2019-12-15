// Minote - game.cpp

#include "game.h"
#include "log.h"

Game::Game(Window& w)
		:window{w},
		 thread{&Game::run, this}
{ }

Game::~Game()
{
	thread.join();
}

auto Game::run() -> void
{
	while (window.isOpen()) {
		while (auto i = window.popInput()) {
			if (i->action != GLFW_PRESS)
				continue;
			Log::debug("Key pressed: ", i->key, " at ", i->timestamp);
			if (i->key == GLFW_KEY_ESCAPE)
				window.close();
		}
	}
}

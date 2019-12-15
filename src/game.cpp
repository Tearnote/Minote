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
		pushState(std::make_unique<Gameplay>(*this));

		while (window.isOpen()) {
			while (auto i = window.popInput()) {
				if (i->action != GLFW_PRESS)
					continue;
				Log::debug("Key pressed: ", i->key, " at ", i->timestamp);
				if (i->key == GLFW_KEY_ESCAPE)
					window.close();
			}

			updateStates();
			renderStates(renderer);

			renderer.render();
		}
	}
	catch (std::exception &e) {
		Log::crit("Exception caught: ", e.what());
		Log::disable(Log::File);
		window.close();
	}
}

auto Game::pushState(std::unique_ptr<State> s) -> void
{
	stateStack.push_back(std::move(s));
}

auto Game::updateStates() -> void
{
	for(gsl::index i{0}; i < stateStack.size(); i++) {
		auto& s{stateStack[i]};
		bool active{i + 1 == stateStack.size()};
		auto result{s->update(active)};
		if (result == State::Delete) {
			if (!active)
				throw std::logic_error{"State #"s + std::to_string(i) + " requested delete while being inactive"s};
			stateStack.pop_back();
		}
	}
}

auto Game::renderStates(Renderer& r) const -> void
{
	for(const auto& s: stateStack) {
		bool active{s == stateStack.back()};
		s->render(active, r);
	}
}

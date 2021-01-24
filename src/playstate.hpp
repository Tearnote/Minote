#pragma once

#include <span>
#include "gfx/engine.hpp"
#include "mapper.hpp"
#include "mino.hpp"

namespace minote {

struct PlayState {

	using Action = engine::Mapper::Action;

	PlayState();
	void tick(std::span<Action const> actions);
	void draw(gfx::Engine& engine);

private:

	Grid<10, 22> m_grid;

};

}

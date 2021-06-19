#pragma once

#include <optional>
#include <utility>
#include "base/container/sarray.hpp"
#include "base/memory/arena.hpp"
#include "base/types.hpp"
#include "base/util.hpp"

namespace minote::base {

using namespace literals;

struct Pool {
	
	static constexpr auto MaxSlots = 8_zu;
	
	Pool(): arenas(MaxSlots) {}
	
	void attach(usize slot, Arena&& arena) { arenas[slot] = std::move(arena); }
	
	auto at(usize slot) -> Arena& { return *arenas[slot]; }
	
private:
	
	sarray<std::optional<Arena>, MaxSlots> arenas;
	
};

}

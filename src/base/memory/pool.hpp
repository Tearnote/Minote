#pragma once

#include <variant>
#include <utility>
#include "base/container/sarray.hpp"
#include "base/memory/stack.hpp"
#include "base/memory/arena.hpp"
#include "base/types.hpp"
#include "base/util.hpp"

namespace minote::base {

using namespace literals;

struct Pool {
	
	static constexpr auto MaxSlots = 8_zu;
	
	Pool(): buffers(MaxSlots) {}
	
	template<typename T>
	void attach(usize slot, T&& buffer) { buffers[slot] = std::move(buffer); }
	
	template<typename T>
	auto at(usize slot) -> T& { return std::get<T>(buffers[slot]); }
	
private:
	
	sarray<std::variant<
		std::monostate,
		Arena,
		Stack
	>, MaxSlots> buffers;
	
};

}

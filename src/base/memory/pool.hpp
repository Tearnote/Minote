#pragma once

#include <variant>
#include <utility>
#include "base/container/array.hpp"
#include "base/memory/stack.hpp"
#include "base/memory/arena.hpp"
#include "base/types.hpp"
#include "base/util.hpp"

namespace minote::base {

using namespace literals;

// A collection of memory resources to be used for allocation. A number of
// indexed slots is available, each one can contain any type of memory resource.
// Care must be taken that slots are accessed as the same memory resource type
// that was previously attached.
struct Pool {
	
	static constexpr auto MaxSlots = 8_zu;
	
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

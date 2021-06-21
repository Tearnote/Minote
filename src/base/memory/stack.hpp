#pragma once

#include <string_view>
#include "base/memory/arena.hpp"
#include "base/types.hpp"

namespace minote::base {

struct StackMarker;

// Memory resource for a stack-like allocator. A marker can be used to free
// multiple allocations at once.
struct Stack: public Arena {
	
	Stack(std::string_view name, usize capacity): Arena(name, capacity) {}
	
	friend StackMarker;
	
};

// A RAII marker for a Stack memory resource. All allocations made on the memory
// resource since the marker was created are freed when the scope ends.
struct StackMarker {
	
	explicit StackMarker(Stack& stack): stack(stack), marker(stack.used) {}
	~StackMarker() { stack.used = marker; }
	
	// Not copyable
	// Not moveable
	StackMarker(StackMarker const&) = delete;
	auto operator=(StackMarker const&) -> StackMarker& = delete;
	
private:
	
	Stack& stack;
	usize marker;
	
};

}

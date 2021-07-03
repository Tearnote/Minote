#pragma once

#include "base/container/string.hpp"
#include "base/memory/arena.hpp"
#include "base/types.hpp"

namespace minote::base {

struct StackMarker;

// Memory resource for a stack-like allocator. A marker can be used to free
// multiple allocations at once.
struct Stack: public Arena {
	
	Stack(string_view name, usize capacity): Arena(name, capacity) {}
	
	friend StackMarker;
	
};

// A RAII marker for a Stack memory resource. All allocations made on the memory
// resource since the marker was created are freed when the scope ends.
struct StackMarker {
	
	explicit StackMarker(Stack& stack): m_stack(stack), m_marker(stack.m_used) {}
	~StackMarker() { m_stack.m_used = m_marker; }
	
	// Not copyable
	// Not moveable
	StackMarker(StackMarker const&) = delete;
	auto operator=(StackMarker const&) -> StackMarker& = delete;
	
private:
	
	Stack& m_stack;
	usize m_marker;
	
};

}

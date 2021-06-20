#pragma once

#include <string_view>
#include "base/memory/arena.hpp"
#include "base/types.hpp"

namespace minote::base {

struct StackMarker;

struct Stack: public Arena {
	
	Stack(std::string_view name, usize capacity): Arena(name, capacity) {}
	
	friend StackMarker;
	
};

struct StackMarker {
	
	explicit StackMarker(Stack& stack): stack(stack), marker(stack.used) {}
	~StackMarker() { stack.used = marker; }
	
	StackMarker(StackMarker const&) = delete;
	auto operator=(StackMarker const&) -> StackMarker& = delete;
	
private:
	
	Stack& stack;
	usize marker;
	
};

}

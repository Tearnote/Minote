#pragma once

#include "base/container/string.hpp"
#include "base/types.hpp"

namespace minote::base {

// Memory resource for a linear allocator. The only way to free space is to
// reset the entire resource, freeing every allocation at once.
struct Arena {
	
	Arena(string_view name, usize capacity);
	~Arena();
	
	auto allocate(usize bytes, usize align) -> void*;
	
	void reset();
	
	// Not copyable
	Arena(Arena const&) = delete;
	auto operator=(Arena const&) -> Arena& = delete;
	// Moveable
	Arena(Arena&&);
	auto operator=(Arena&&) -> Arena&;
	
protected:
	
	sstring name;
	void* mem;
	usize capacity;
	usize used;
	
};

}

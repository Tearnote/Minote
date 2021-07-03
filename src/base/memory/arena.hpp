#pragma once

#include "base/container/string.hpp"
#include "base/types.hpp"

namespace minote::base {

// Memory resource for a linear allocator. The only way to free space is to
// reset the entire resource, freeing every allocation at once.
struct Arena {
	
	Arena(string_view name, usize capacity);
	~Arena();
	
	// Perform aligned allocation. Returned memory is uninitialized
	auto allocate(usize bytes, usize align) -> void*;
	
	// Free everything
	void reset();
	
	// Not copyable
	Arena(Arena const&) = delete;
	auto operator=(Arena const&) -> Arena& = delete;
	// Moveable
	Arena(Arena&&);
	auto operator=(Arena&&) -> Arena&;
	
protected:
	
	sstring m_name;
	void* m_mem;
	usize m_capacity;
	usize m_used;
	
};

}

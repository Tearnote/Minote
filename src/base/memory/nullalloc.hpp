#pragma once

#include "base/error.hpp"
#include "base/types.hpp"

namespace minote::base {

// An STL-compatible allocator that throws on any allocation. Useful for
// ensuring that a container doesn't allocate from heap.
template<typename T>
struct NullAllocator {
	
	using value_type = T;
	
	auto allocate(usize) -> T* { throw runtime_error("Requested memory from NullAllocator"); }
	void deallocate(T*, usize) {}
	
	// Boilerplate below.
	
	auto operator==(NullAllocator const&) const -> bool { return true; }
	auto operator!=(NullAllocator const&) const -> bool = default;
	
	template<typename U>
	struct rebind { typedef NullAllocator<U> other; };
	NullAllocator() = default;
	template<typename U>
	NullAllocator(NullAllocator<U>) {}
	
};

}

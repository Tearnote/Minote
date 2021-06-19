#pragma once

#include <stdexcept>
#include "base/types.hpp"

namespace minote::base {

template<typename T>
struct NullAllocator {
	
	using value_type = T;
	
	auto allocate(usize) -> T* { throw std::runtime_error("Requested memory from NullAllocator"); }
	void deallocate(T*, usize) {}
	
	auto operator==(NullAllocator const&) const -> bool { return true; }
	auto operator!=(NullAllocator const&) const -> bool = default;
	
	template<typename U>
	struct rebind { typedef NullAllocator<U> other; };
	NullAllocator() = default;
	template<typename U>
	NullAllocator(NullAllocator<U>) {}
	
};

}

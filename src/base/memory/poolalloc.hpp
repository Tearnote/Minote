#pragma once

#include "base/memory/pool.hpp"
#include "base/types.hpp"

namespace minote::base {

// An STL-compatible allocator that uses a memory resource that was previously
// attached to a Pool.
template<typename T, Pool& P, usize Slot, typename Buf>
struct PoolAllocator {
	
	using value_type = T;
	
	static constexpr auto Size = sizeof(T);
	static constexpr auto Align = alignof(T);
	
	auto allocate(usize elem) -> T* { return (T*)(P.at<Buf>(Slot).allocate(Size * elem, Align)); }
	void deallocate(T*, usize) {}
	
	// Boilerplate below.
	
	auto operator==(PoolAllocator<T, P, Slot, Buf> const&) const -> bool { return true; }
	auto operator!=(PoolAllocator<T, P, Slot, Buf> const&) const -> bool = default;
	
	template<typename U>
	struct rebind { typedef PoolAllocator<U, P, Slot, Buf> other; };
	PoolAllocator() = default;
	template<typename U>
	PoolAllocator(PoolAllocator<U, P, Slot, Buf>) {}
	
};

}

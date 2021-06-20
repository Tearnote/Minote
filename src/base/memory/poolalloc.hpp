#pragma once

#include "base/memory/pool.hpp"
#include "base/types.hpp"

namespace minote::base {

template<typename T, Pool& P, usize Slot, typename Buf>
struct PoolAllocator {
	
	using value_type = T;
	
	auto allocate(usize elem) -> T* { return (T*)(P.at<Buf>(Slot).allocate(sizeof(T) * elem, alignof(T))); }
	void deallocate(T*, usize) {}
	
	auto operator==(PoolAllocator<T, P, Slot, Buf> const&) const -> bool { return true; }
	auto operator!=(PoolAllocator<T, P, Slot, Buf> const&) const -> bool = default;
	
	template<typename U>
	struct rebind { typedef PoolAllocator<U, P, Slot, Buf> other; };
	PoolAllocator() = default;
	template<typename U>
	PoolAllocator(PoolAllocator<U, P, Slot, Buf>) {}
	
};

}

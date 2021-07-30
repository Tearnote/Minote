#pragma once

#include "base/memory/poolalloc.hpp"
#include "base/memory/arena.hpp"
#include "base/memory/stack.hpp"
#include "base/memory/pool.hpp"
#include "base/util.hpp"

namespace minote {

using namespace base;
using namespace base::literals;

enum struct PoolSlot {
	Permanent = 0,
	PerFrame = 1,
	Scratch = 2,
	MaxSlot = Pool::MaxSlots,
};

inline auto GlobalPool = Pool();

template<typename T>
using PerFrame = PoolAllocator<T, GlobalPool, +PoolSlot::PerFrame, Arena>;

template<typename T>
using Scratch = PoolAllocator<T, GlobalPool, +PoolSlot::Scratch, Stack>;

struct ScratchMarker {
	
	ScratchMarker(): marker(GlobalPool.at<Stack>(+PoolSlot::Scratch)) {}
	
private:
	
	StackMarker marker;
	
};

template<typename T>
using StdAlloc = std::allocator<T>;

inline void attachPoolResources() {
	
	GlobalPool.attach(+PoolSlot::PerFrame, Arena("Per-frame", 16_mb));
	GlobalPool.attach(+PoolSlot::Scratch, Stack("Scratch", 16_mb));
	
}

}

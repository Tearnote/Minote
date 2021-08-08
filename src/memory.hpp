#pragma once

#include "base/memory/poolalloc.hpp"
#include "base/memory/arena.hpp"
#include "base/memory/stack.hpp"
#include "base/memory/pool.hpp"
#include "base/util.hpp"

namespace minote {

using namespace base;
using namespace base::literals;

// Mapping of memory allocator usage in the pool
enum struct PoolSlot: usize {
	Permanent = 0,
	PerFrame = 1,
	Scratch = 2,
	MaxSlot = Pool::MaxSlots,
};

inline auto GlobalPool = Pool();

// Bump allocator, whose memory is freed at the end of each frame
template<typename T>
using PerFrame = PoolAllocator<T, GlobalPool, +PoolSlot::PerFrame, Arena>;

// Stack allocator, to be used with a ScratchMarker at the start of a scope
template<typename T>
using Scratch = PoolAllocator<T, GlobalPool, +PoolSlot::Scratch, Stack>;

// Marker struct that frees all scratch memory allocated since its creation
// when it's destroyed
struct ScratchMarker {
	
	ScratchMarker(): m_marker(GlobalPool.at<Stack>(+PoolSlot::Scratch)) {}
	
private:
	
	StackMarker m_marker;
	
};

// Initialize all memory allocators
inline void attachPoolResources() {
	
	GlobalPool.attach(+PoolSlot::PerFrame, Arena("Per-frame", 16_mb));
	GlobalPool.attach(+PoolSlot::Scratch, Stack("Scratch", 32_mb));
	
}

// Call at the end of a frame to free all per-frame memory at once
inline void resetPerFrameAllocator() {
	
	GlobalPool.at<Arena>(+PoolSlot::PerFrame).reset();
	
}

}

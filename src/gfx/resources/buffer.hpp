#pragma once

#include <span>
#include "vuk/RenderGraph.hpp"
#include "vuk/Buffer.hpp"
#include "vuk/Name.hpp"
#include "base/types.hpp"
#include "gfx/resources/pool.hpp"

namespace minote::gfx {

using namespace base;

template<typename T>
struct Buffer {
	
	vuk::Name name;
	vuk::Buffer* handle = nullptr;
	
	// Construct an empty buffer inside a pool. If the pool already contained a buffer under
	// the same name, the existing one is retrieved instead.
	static auto make(Pool&, vuk::Name, vuk::BufferUsageFlags, usize elements = 1,
		vuk::MemoryUsage = vuk::MemoryUsage::eGPUonly) -> Buffer<T>;
	
	// Construct a buffer inside a pool and transfer data into it. If the pool already contained
	// a buffer under the same name, the existing one is retrieved instead, but the transfer
	// still proceeds. For GPU-only buffers, the transfer is not waited for.
	static auto make(Pool&, vuk::Name, vuk::BufferUsageFlags, std::span<T const> data,
		vuk::MemoryUsage = vuk::MemoryUsage::eCPUtoGPU) -> Buffer<T>;
	
	// Size of the buffer in bytes.
	[[nodiscard]]
	auto size() const -> usize { return handle->size; }
	
	// Declare as a vuk::Resource.
	[[nodiscard]]
	auto resource(vuk::Access) const -> vuk::Resource;
	
	// Attach the buffer to the rendergraph.
	void attach(vuk::RenderGraph&, vuk::Access initial, vuk::Access final);
	
	// Convertible to vuk::Buffer
	operator vuk::Buffer() const { return *handle; }

	
};

}

#include "gfx/resources/buffer.tpp"

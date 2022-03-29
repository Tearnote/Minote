#pragma once

#include <span>
#include "vuk/RenderGraph.hpp"
#include "vuk/Buffer.hpp"
#include "vuk/Name.hpp"
#include "base/types.hpp"
#include "gfx/resources/pool.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;

template<typename T>
struct Buffer {
	
	using value_type = T;
	
	vuk::Name name;
	vuk::Buffer* handle = nullptr;
	
	// Construct an empty buffer inside a pool. If the pool already contained a buffer under
	// the same name, the existing one is retrieved instead.
	static auto make(Pool&, vuk::Name, vuk::BufferUsageFlags, usize elements = 1,
		vuk::MemoryUsage = vuk::MemoryUsage::eGPUonly) -> Buffer<T>;
	
	// Construct a buffer inside a pool and transfer data into it. If the pool already contained
	// a buffer under the same name, the existing one is retrieved instead, but the transfer
	// still proceeds. Setting elementCapacity allows for a buffer larger than provided data.
	static auto make(Pool&, vuk::Name, vuk::BufferUsageFlags, std::span<T const> data, usize elementCapacity = 0_zu) -> Buffer<T>;
	
	// Create a buffer reference that starts at the specified element count.
	auto offsetView(usize elements) const -> vuk::Buffer;
	
	// Size of the buffer in bytes.
	[[nodiscard]]
	auto size() const -> usize { return handle->size; }
	
	// Number of elements in the buffer.
	auto length() const -> usize { return size() / sizeof(value_type); }
	
	[[nodiscard]]
	auto mappedPtr() -> T* { return reinterpret_cast<T*>(handle->mapped_ptr); }
	
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

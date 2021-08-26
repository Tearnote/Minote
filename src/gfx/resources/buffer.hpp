#pragma once

#include <span>
#include "vuk/Context.hpp"
#include "vuk/Buffer.hpp"
#include "base/types.hpp"

namespace minote::gfx {

using namespace base;

// Type-safe buffer wrapper. Convertible to vuk::Buffer, but helps ensure
// type safety in resource passing.
template<typename T>
struct Buffer {
	
	vuk::Name name;
	vuk::Buffer handle;
	
	// Construct an invalid buffer. It will destruct safely, but the only
	// meaningful use is to overwrite it later.
	Buffer() = default;
	
	// Construct an empty buffer.
	Buffer(vuk::PerThreadContext&, vuk::Name, vuk::BufferUsageFlags,
		usize elements = 1, vuk::MemoryUsage = vuk::MemoryUsage::eGPUonly);
	
	// Construct a buffer with the given data. If memory usage is GPU only,
	// a transfer will be queued but not waited for.
	Buffer(vuk::PerThreadContext&, vuk::Name, std::span<T const> data,
		vuk::BufferUsageFlags, vuk::MemoryUsage = vuk::MemoryUsage::eCPUtoGPU);
	
	// Destroy the buffer after the current frame is fully finished drawing.
	// If the buffer is valid, this must be called.
	void recycle(vuk::PerThreadContext&);
	
	// Size of the buffer in bytes.
	auto size() const -> usize { return handle.size; }
	
	// Not copyable, moveable
	Buffer(Buffer<T> const&) = delete;
	auto operator=(Buffer<T> const&) -> Buffer<T>& = delete;
	Buffer(Buffer<T>&&);
	auto operator=(Buffer<T>&&) -> Buffer<T>&;
	
	// Convertible to vuk::Buffer
	operator vuk::Buffer() const { return handle; }
	
};

}

#include "gfx/resources/buffer.tpp"
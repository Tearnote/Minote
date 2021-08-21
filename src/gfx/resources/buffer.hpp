#pragma once

#include "vuk/Context.hpp"
#include "vuk/Buffer.hpp"
#include "base/types.hpp"

namespace minote::gfx {

using namespace base;

template<typename T>
struct Buffer {
	
	vuk::Name name;
	vuk::Buffer handle;
	
	// Construct a buffer in shared memory with the given data.
	Buffer(vuk::PerThreadContext&, vuk::Name, T const& data, vuk::BufferUsageFlags);
	
	// Recycle the buffer. The underlying handle is still safe to use in lambdas
	// and such until the same in-flight frame is started again.
	~Buffer();
	
	// Size of the buffer in bytes.
	auto size() const -> usize { return handle.size; }
	
	// Not copyable, moveable
	Buffer(Buffer<T> const&) = delete;
	auto operator=(Buffer<T> const&) -> Buffer<T>& = delete;
	Buffer(Buffer<T>&&);
	auto operator=(Buffer<T>&&) -> Buffer<T>&;
	
	// Convertible to vuk::Buffer
	operator vuk::Buffer() const { return handle; }
	
private:
	
	// Referenced on destruction
	vuk::PerThreadContext& m_ptc;
	
};

}

#include "gfx/resources/buffer.tpp"

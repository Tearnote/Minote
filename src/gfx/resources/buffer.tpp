#pragma once

namespace minote::gfx {

template<typename T>
Buffer<T>::Buffer(vuk::PerThreadContext& _ptc, vuk::Name _name, T const& _data,
	vuk::BufferUsageFlags _usage):
	name(_name), m_ptc(_ptc) {
	
	handle = _ptc.allocate_buffer(vuk::MemoryUsage::eCPUtoGPU, _usage,
		sizeof(T), alignof(T)).release();
	
	std::memcpy(handle.mapped_ptr, &_data, sizeof(T));
	
}

template<typename T>
Buffer<T>::~Buffer() {
	
	// Only destroy valid buffers
	if (!handle) return;
	
	m_ptc.ctx.enqueue_destroy(handle);
	
}

template<typename T>
Buffer<T>::Buffer(Buffer<T>&& _other):
	m_ptc(_other.m_ptc) {
	
	*this = std::move(_other);
	
}

template<typename T>
auto Buffer<T>::operator=(Buffer<T>&& _other) -> Buffer<T>& {
	
	name = _other.name;
	handle = _other.handle;
	
	// Invalidate the other buffer
	_other.handle.buffer = VK_NULL_HANDLE;
	
}

}

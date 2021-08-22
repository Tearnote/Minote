#pragma once

#include <cassert>
#include "base/util.hpp"

namespace minote::gfx {

using namespace base::literals;

template<typename T>
Buffer<T>::Buffer(vuk::PerThreadContext& _ptc, vuk::Name _name,
	vuk::BufferUsageFlags _usage, usize _elements, vuk::MemoryUsage _memUsage) {
	
	name = _name;
	handle = _ptc.allocate_buffer(_memUsage, _usage,
		sizeof(T) * _elements, alignof(T)).release();
	
}

template<typename T>
Buffer<T>::Buffer(vuk::PerThreadContext& _ptc, vuk::Name _name,
	std::span<T const> _data, vuk::BufferUsageFlags _usage,
	vuk::MemoryUsage _memUsage) {
	
	assert(_memUsage == vuk::MemoryUsage::eCPUtoGPU ||
	       _memUsage == vuk::MemoryUsage::eGPUonly);
	
	name = _name;
	
	if (_memUsage == vuk::MemoryUsage::eGPUonly)
		_usage |= vuk::BufferUsageFlagBits::eTransferDst;
	
	handle = _ptc.allocate_buffer(_memUsage, _usage,
		_data.size_bytes(), alignof(T)).release();
	
	if (_memUsage == vuk::MemoryUsage::eCPUtoGPU)
		std::memcpy(handle.mapped_ptr, _data.data(), _data.size_bytes());
	else
		_ptc.upload(handle, _data);
	
}

template<typename T>
void Buffer<T>::recycle(vuk::PerThreadContext& _ptc) {
	
	assert(handle);
	
	_ptc.ctx.enqueue_destroy(handle);
	handle.buffer = VK_NULL_HANDLE;
	
}

template<typename T>
Buffer<T>::Buffer(Buffer<T>&& _other) {
	
	*this = std::move(_other);
	
}

template<typename T>
auto Buffer<T>::operator=(Buffer<T>&& _other) -> Buffer<T>& {
	
	name = _other.name;
	handle = _other.handle;
	
	// Invalidate the other buffer
	_other.handle.buffer = VK_NULL_HANDLE;
	
	return *this;
	
}

}

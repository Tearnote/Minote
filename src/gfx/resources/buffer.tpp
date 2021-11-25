#include "gfx/resources/buffer.hpp"

#include <cassert>
#include <cstring>

namespace minote::gfx {

template<typename T>
auto Buffer<T>::make(Pool& _pool, vuk::Name _name, vuk::BufferUsageFlags _usage,
	usize _elements, vuk::MemoryUsage _memUsage) -> Buffer<T> {
	
	assert(_memUsage == vuk::MemoryUsage::eCPUtoGPU ||
	       _memUsage == vuk::MemoryUsage::eGPUonly);
	
	auto& buffer = [&_pool, _name, _elements, _usage, _memUsage]() -> vuk::Buffer& {
		
		if (_pool.contains(_name)) {
			
			return *_pool.get<vuk::Unique<vuk::Buffer>>(_name);
			
		} else {
			
			auto size = sizeof(T) * _elements;
			return *_pool.insert<vuk::Unique<vuk::Buffer>>(_name,
				_pool.ptc().allocate_buffer(_memUsage, _usage, size, alignof(T)));
			
		}
		
	}();
	
	return Buffer<T>{
		.name = _name,
		.handle = &buffer };
	
}

template<typename T>
auto Buffer<T>::make(Pool& _pool, vuk::Name _name, vuk::BufferUsageFlags _usage,
	std::span<T const> _data, usize _elementCapacity) -> Buffer<T> {
	
	assert(_elementCapacity == 0 || _elementCapacity >= _data.size());
	
	auto& buffer = [&_pool, _name, &_data, _usage, _elementCapacity]() -> vuk::Buffer& {
		
		if (_pool.contains(_name)) {
			
			return *_pool.get<vuk::Unique<vuk::Buffer>>(_name);
			
		} else {
			
			auto size = _data.size_bytes();
			if (_elementCapacity != 0)
				size = _elementCapacity * sizeof(T);
			
			return *_pool.insert<vuk::Unique<vuk::Buffer>>(_name,
				_pool.ptc().allocate_buffer(vuk::MemoryUsage::eCPUtoGPU, _usage, size, alignof(T)));
			
		}
		
	}();
	
	assert(buffer.size >= _data.size_bytes());
	
	std::memcpy(buffer.mapped_ptr, _data.data(), _data.size_bytes());
	
	return Buffer<T>{
		.name = _name,
		.handle = &buffer };
	
}

template<typename T>
auto Buffer<T>::offsetView(usize _elements) const -> vuk::Buffer {
	
	auto result = *handle;
	result.offset += _elements * sizeof(T);
	return result;
	
}

template<typename T>
auto Buffer<T>::resource(vuk::Access _access) const -> vuk::Resource {
	
	return vuk::Resource(name, vuk::Resource::Type::eBuffer, _access);
	
}

template<typename T>
void Buffer<T>::attach(vuk::RenderGraph& _rg, vuk::Access _initial, vuk::Access _final) {
	
	_rg.attach_buffer(name, *handle, _initial, _final);
	
}
	
}

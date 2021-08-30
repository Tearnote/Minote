#include <cassert>
#include <cstring>
#include "gfx/util.hpp"

namespace minote::gfx {

template<typename T>
requires std::is_same_v<T, Texture2D>
auto ResourcePool::make_texture(vuk::Name _name, uvec2 _size,
	vuk::Format _format, vuk::ImageUsageFlags _usage, u32 _mips) -> Texture2D {
	
	auto& texture = [&, this]() -> vuk::Texture& {
		
		if (m_resources.contains(_name)) {
			
			return std::get<vuk::Texture>(m_resources.at(_name));
			
		} else {
			
			return std::get<vuk::Texture>(m_resources.try_emplace(_name,
				std::in_place_type<vuk::Texture>,
				m_ptc->allocate_texture(vuk::ImageCreateInfo{
					.format = _format,
					.extent = {_size.x(), _size.y(), 1},
					.mipLevels = _mips,
					.usage = _usage })).first->second);
			
		}
		
	}();
	
	return Texture2D{
		.name = _name,
		.handle = &texture };
	
}

template<typename T>
requires std::is_same_v<T, Cubemap>
auto ResourcePool::make_texture(vuk::Name _name, u32 _size,
	vuk::Format _format, vuk::ImageUsageFlags _usage) -> Cubemap {
	
	auto& texture = [&, this]() -> vuk::Texture& {
		
		if (m_resources.contains(_name)) {
			
			return std::get<vuk::Texture>(m_resources.at(_name));
			
		} else {
			
			auto& result = std::get<vuk::Texture>(m_resources.try_emplace(_name,
				std::in_place_type<vuk::Texture>,
				m_ptc->allocate_texture(vuk::ImageCreateInfo{
					.flags = vuk::ImageCreateFlagBits::eCubeCompatible,
					.format = _format,
					.extent = {_size, _size, 1},
					.mipLevels = mipmapCount(_size),
					.arrayLayers = 6,
					.usage = _usage })).first->second);
			result.view = m_ptc->create_image_view(vuk::ImageViewCreateInfo{
				.image = *result.image,
				.viewType = vuk::ImageViewType::eCube,
				.format = result.format,
				.subresourceRange = vuk::ImageSubresourceRange{
					.aspectMask = vuk::ImageAspectFlagBits::eColor,
					.levelCount = VK_REMAINING_MIP_LEVELS,
					.layerCount = 6 }});
			
			return result;
			
		}
		
	}();
	
	auto result = Cubemap();
	result.name = _name;
	result.handle = &texture;
	return result;
	
}

template<typename T>
auto ResourcePool::make_buffer(vuk::Name _name, vuk::BufferUsageFlags _usage,
	usize _elements, vuk::MemoryUsage _memUsage) -> Buffer<T> {
	
	assert(_memUsage == vuk::MemoryUsage::eCPUtoGPU ||
	       _memUsage == vuk::MemoryUsage::eGPUonly);
	
	auto& buffer = [&, this]() -> vuk::Buffer& {
		
		if (m_resources.contains(_name)) {
			
			return *std::get<vuk::Unique<vuk::Buffer>>(m_resources.at(_name));
			
		} else {
			
			auto size = sizeof(T) * _elements;
			return *std::get<vuk::Unique<vuk::Buffer>>(m_resources.try_emplace(_name,
				std::in_place_type<vuk::Unique<vuk::Buffer>>,
				m_ptc->allocate_buffer(_memUsage, _usage, size, alignof(T))).first->second);
				
		}
		
	}();
	
	return Buffer<T>{
		.name = _name,
		.handle = &buffer };
	
}

template<typename T>
auto ResourcePool::make_buffer(vuk::Name _name, vuk::BufferUsageFlags _usage,
	std::span<T const> _data, vuk::MemoryUsage _memUsage) -> Buffer<T> {
	
	assert(_memUsage == vuk::MemoryUsage::eCPUtoGPU ||
	       _memUsage == vuk::MemoryUsage::eGPUonly);
	
	auto& buffer = [&, this]() -> vuk::Buffer& {
		
		if (m_resources.contains(_name)) {
			
			return *std::get<vuk::Unique<vuk::Buffer>>(m_resources.at(_name));
			
		} else {
			
			if (_memUsage == vuk::MemoryUsage::eGPUonly)
				_usage |= vuk::BufferUsageFlagBits::eTransferDst;
			
			auto& result = *std::get<vuk::Unique<vuk::Buffer>>(m_resources.try_emplace(_name,
				std::in_place_type<vuk::Unique<vuk::Buffer>>,
				m_ptc->allocate_buffer(_memUsage, _usage, _data.size_bytes(), alignof(T))).first->second);
			
			return result;
			
		}
		
	}();
	
	assert(buffer.size <= _data.size_bytes());
	
	if (_memUsage == vuk::MemoryUsage::eCPUtoGPU)
		std::memcpy(buffer.mapped_ptr, _data.data(), _data.size_bytes());
	else
		m_ptc->upload(buffer, _data);
	
	return Buffer<T>{
		.name = _name,
		.handle = &buffer };
	
}

}

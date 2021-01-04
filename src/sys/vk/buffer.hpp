#pragma once

#include <span>
#include "base/concepts.hpp"
#include "base/types.hpp"
#include "VulkanMemoryAllocator/vma.h"
#include "volk/volk.h"

namespace minote::sys::vk {

struct Buffer {

	VkBuffer buffer;
	VmaAllocation allocation;
	base::size_t size;

};

auto createBuffer(VmaAllocator allocator, base::size_t size,
	VkBufferUsageFlags usage, VmaMemoryUsage memUsage) -> Buffer;

template<base::trivially_copyable T>
void uploadToCpuBuffer(VmaAllocator allocator, Buffer& buffer, T const& data);

template<base::trivially_copyable T>
void uploadToCpuBuffer(VmaAllocator allocator, Buffer& buffer, std::span<T const> data);

}

#include "sys/vk/buffer.tpp"

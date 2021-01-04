#pragma once

#include <cstring>
#include <span>
#include "VulkanMemoryAllocator/vma.h"
#include "base/assert.hpp"
#include "sys/vk/base.hpp"

namespace minote::sys::vk {

template<base::trivially_copyable T>
void uploadToCpuBuffer(VmaAllocator allocator, Buffer& buffer, T const& data) {
	ASSERT(buffer.size >= sizeof(T));

	void* mappedStorage;
	VK(vmaMapMemory(allocator, buffer.allocation, &mappedStorage));
	std::memcpy(mappedStorage, &data, sizeof(T));
	vmaUnmapMemory(allocator, buffer.allocation);
}

template<base::trivially_copyable T>
void uploadToCpuBuffer(VmaAllocator allocator, Buffer& buffer, std::span<T const> data) {
	ASSERT(buffer.size >= data.size_bytes());

	void* mappedStorage;
	VK(vmaMapMemory(allocator, buffer.allocation, &mappedStorage));
	std::memcpy(mappedStorage, data.data(), data.size_bytes());
	vmaUnmapMemory(allocator, buffer.allocation);
}

}

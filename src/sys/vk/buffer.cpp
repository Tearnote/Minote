#include "sys/vk/buffer.hpp"

#include "VulkanMemoryAllocator/vma.h"
#include "volk/volk.h"
#include "base/types.hpp"
#include "sys/vk/base.hpp"

namespace minote::sys::vk {

using namespace base;

auto createBuffer(VmaAllocator allocator, size_t size,
	VkBufferUsageFlags usage, VmaMemoryUsage memUsage) -> Buffer {
	auto const bufferCI = VkBufferCreateInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage,
	};
	auto const allocationCI = VmaAllocationCreateInfo{
		.usage = memUsage,
	};

	Buffer result;
	VK(vmaCreateBuffer(allocator, &bufferCI, &allocationCI, &result.buffer, &result.allocation, nullptr));
	result.size = size;
	return result;
}

}

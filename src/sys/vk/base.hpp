#pragma once

#include <stdexcept>
#include <cstdint>
#include <fmt/core.h>
#include "volk/volk.h"

namespace minote::sys::vk {

#ifndef NDEBUG
#define VK_VALIDATION
#endif //NDEBUG

#define VK(x) \
	do { \
		auto err = (x); \
		if (err != VK_SUCCESS) \
			throw std::runtime_error{fmt::format("{}:{} Vulkan non-zero return code: #{}", __FILE__, __LINE__, err)}; \
	} while (false)

#ifdef VK_VALIDATION
template<typename T>
void setDebugName(VkDevice device, T handle, VkObjectType type, std::string_view name) {
	auto const sName = std::string{name};
	auto const objectNameInfo = VkDebugUtilsObjectNameInfoEXT{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		.objectType = type,
		.objectHandle = reinterpret_cast<std::uint64_t>(handle),
		.pObjectName = sName.c_str(),
	};
	VK(vkSetDebugUtilsObjectNameEXT(device, &objectNameInfo));
}
#else //VK_VALIDATION
template<typename T>
void setDebugName(VkDevice, T, VkObjectType, std::string_view) {}
#endif //VK_VALIDATION

}

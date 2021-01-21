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

}

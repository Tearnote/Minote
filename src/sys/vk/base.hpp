#pragma once

#include <stdexcept>
#include <fmt/core.h>

namespace minote::sys::vk {

#define VK(x) \
	do { \
		auto err = (x); \
		if (err != VK_SUCCESS) \
			throw std::runtime_error{fmt::format("{}:{} Vulkan non-zero return code: #{}", __FILE__, __LINE__, err)}; \
	} while (false)

}

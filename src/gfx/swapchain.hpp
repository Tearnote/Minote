#pragma once

#include <vector>
#include "volk/volk.h"
#include "sys/vk/image.hpp"
#include "gfx/context.hpp"

namespace minote::gfx {

struct Swapchain {

	VkSwapchainKHR swapchain = nullptr;
	VkExtent2D extent = {};
	std::vector<sys::vk::Image> color;

	void init(Context& ctx, VkSwapchainKHR old = nullptr);
	void cleanup(Context& ctx);

};

}

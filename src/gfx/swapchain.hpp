#pragma once

#include <utility>
#include <vector>
#include "volk/volk.h"
#include "sys/vk/image.hpp"
#include "gfx/commands.hpp"
#include "gfx/context.hpp"

namespace minote::gfx {

using namespace base;

struct Swapchain {

	VkSwapchainKHR swapchain = nullptr;
	VkExtent2D extent = {};
	std::vector<sys::vk::Image> color;

	void init(Context& ctx, VkSwapchainKHR old = nullptr);
	void cleanup(Context& ctx);

	auto acquire(Context& ctx, Commands::Frame& frame) -> std::pair<u32, VkResult>;

};

}

#pragma once

#include "volk/volk.h"
#include "gfx/context.hpp"
#include "gfx/base.hpp"

namespace minote::gfx {

struct Commands {

	struct Frame {

		VkCommandPool commandPool;
		VkCommandBuffer commandBuffer;
		VkSemaphore renderSemaphore;
		VkSemaphore presentSemaphore;
		VkFence renderFence;

	};

	PerFrame<Frame> frames;
	VkCommandPool transferCommandPool;
	VkFence transfersFinished;

	void init(Context& ctx);
	void cleanup(Context& ctx);

};

}

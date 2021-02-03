#pragma once

#include <concepts>
#include "volk/volk.h"
#include "gfx/swapchain.hpp"
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

	template<typename F>
		requires std::invocable<F, VkCommandBuffer>
	void transferAsync(Context& ctx, F func);

	template<typename F, typename G>
		requires std::invocable<F>
		&& std::invocable<G, Commands::Frame&, u32, u32>
	void render(Context& ctx, Swapchain& swapchain, u64 frameCount, F refresh, G func);

};

}

#include "gfx/commands.tpp"

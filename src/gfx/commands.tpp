#pragma once

#include "sys/vk/debug.hpp"
#include "base/time.hpp"

namespace minote::gfx {

using namespace base;

template<typename F>
	requires std::invocable<F, VkCommandBuffer>
void Commands::transfer(Context& ctx, F func) {
	namespace vk = sys::vk;

	// Start transfer command recording
	auto commandBufferAI = VkCommandBufferAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = transferCommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	VkCommandBuffer transferCommandBuffer;
	VK(vkAllocateCommandBuffers(ctx.device, &commandBufferAI, &transferCommandBuffer));
	vk::setDebugName(ctx.device, transferCommandBuffer, "Commands::transferCommandBuffer");

	auto commandBufferBI = VkCommandBufferBeginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	VK(vkBeginCommandBuffer(transferCommandBuffer, &commandBufferBI));

	// Record user-provided transfers
	func(transferCommandBuffer);

	// End transfer command recording
	VK(vkEndCommandBuffer(transferCommandBuffer));

	auto submitInfo = VkSubmitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &transferCommandBuffer,
	};
	VK(vkQueueSubmit(ctx.transferQueue, 1, &submitInfo, transfersFinished));

	// Wait for completion and finalize
	vkWaitForFences(ctx.device, 1, &transfersFinished, true, (1_s).count());
	vkResetFences(ctx.device, 1, &transfersFinished);

	vkResetCommandPool(ctx.device, transferCommandPool, 0);
}

}

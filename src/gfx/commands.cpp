#include "gfx/commands.hpp"

#include "sys/vk/debug.hpp"
#include "sys/vk/base.hpp"

namespace minote::gfx {

namespace vk = sys::vk;

void Commands::init(Context& ctx) {
	// Create the graphics command buffers and sync objects
	auto commandPoolCI = VkCommandPoolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = ctx.graphicsQueueFamilyIndex,
	};
	auto commandBufferAI = VkCommandBufferAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	auto fenceCI = VkFenceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT,
	};
	auto const semaphoreCI = VkSemaphoreCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	for (auto& frame: frames) {
		VK(vkCreateCommandPool(ctx.device, &commandPoolCI, nullptr, &frame.commandPool));
		vk::setDebugName(ctx.device, frame.commandPool, fmt::format("Commands::frames[{}].commandPool", &frame - &frames[0]));
		commandBufferAI.commandPool = frame.commandPool;
		VK(vkAllocateCommandBuffers(ctx.device, &commandBufferAI, &frame.commandBuffer));
		vk::setDebugName(ctx.device, frame.commandBuffer, fmt::format("Commands::frames[{}].commandBuffer", &frame - &frames[0]));
		VK(vkCreateFence(ctx.device, &fenceCI, nullptr, &frame.renderFence));
		vk::setDebugName(ctx.device, frame.renderFence, fmt::format("Commands::frames[{}].renderFence", &frame - &frames[0]));
		VK(vkCreateSemaphore(ctx.device, &semaphoreCI, nullptr, &frame.renderSemaphore));
		vk::setDebugName(ctx.device, frame.renderSemaphore, fmt::format("Commands::frames[{}].renderSemaphore", &frame - &frames[0]));
		VK(vkCreateSemaphore(ctx.device, &semaphoreCI, nullptr, &frame.presentSemaphore));
		vk::setDebugName(ctx.device, frame.presentSemaphore, fmt::format("Commands::frames[{}].presentSemaphore", &frame - &frames[0]));
	}

	// Create the transfer queue
	commandPoolCI.queueFamilyIndex = ctx.transferQueueFamilyIndex;
	VK(vkCreateCommandPool(ctx.device, &commandPoolCI, nullptr, &transferCommandPool));
	vk::setDebugName(ctx.device, transferCommandPool, "Commands::transferCommandPool");
	fenceCI.flags = 0;
	VK(vkCreateFence(ctx.device, &fenceCI, nullptr, &transfersFinished));
	vk::setDebugName(ctx.device, transfersFinished, "Commands::transfersFinished");
}

void Commands::cleanup(Context& ctx) {
	vkDestroyFence(ctx.device, transfersFinished, nullptr);
	vkDestroyCommandPool(ctx.device, transferCommandPool, nullptr);
	for (auto& frame: frames) {
		vkDestroySemaphore(ctx.device, frame.presentSemaphore, nullptr);
		vkDestroySemaphore(ctx.device, frame.renderSemaphore, nullptr);
		vkDestroyFence(ctx.device, frame.renderFence, nullptr);
		vkDestroyCommandPool(ctx.device, frame.commandPool, nullptr);
	}
}

}

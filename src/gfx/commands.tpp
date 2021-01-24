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

template<typename F, typename G>
	requires std::invocable<F>
	&& std::invocable<G, Commands::Frame&, u32, u32>
void Commands::render(Context& ctx, Swapchain& swapchain, u64 frameCount, F refresh, G func) {
	// Get the next available frame
	auto const frameIndex = frameCount % FramesInFlight;
	auto& frame = frames[frameIndex];

	// Get the next swapchain image
	auto const swapchainImageIndex = [&, this] {
		while (true) {
			u32 result;
			auto const error = vkAcquireNextImageKHR(ctx.device, swapchain.swapchain,
				std::numeric_limits<u64>::max(), frame.presentSemaphore, nullptr, &result);
			if (error == VK_SUCCESS || error == VK_SUBOPTIMAL_KHR)
				return result;
			if (error == VK_ERROR_OUT_OF_DATE_KHR)
				refresh();
			else
				VK(error);
		}
	}();

	// Wait until the GPU is free
	VK(vkWaitForFences(ctx.device, 1, &frame.renderFence, true, (1_s).count()));
	VK(vkResetFences(ctx.device, 1, &frame.renderFence));

	// Start recording commands
	VK(vkResetCommandBuffer(frame.commandBuffer, 0));
	auto cmdBeginInfo = VkCommandBufferBeginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	VK(vkBeginCommandBuffer(frame.commandBuffer, &cmdBeginInfo));

	// Execute provided drawing commands
	func(frame, frameIndex, swapchainImageIndex);

	// Finish recording commands
	VK(vkEndCommandBuffer(frame.commandBuffer));

	// Submit commands to the queue
	VkPipelineStageFlags const waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	auto const submitInfo = VkSubmitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &frame.presentSemaphore,
		.pWaitDstStageMask = &waitStage,
		.commandBufferCount = 1,
		.pCommandBuffers = &frame.commandBuffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &frame.renderSemaphore,
	};
	VK(vkQueueSubmit(ctx.graphicsQueue, 1, &submitInfo, frame.renderFence));

	// Present the rendered image
	auto const presentInfo = VkPresentInfoKHR{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &frame.renderSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &swapchain.swapchain,
		.pImageIndices = &swapchainImageIndex,
	};
	auto result = vkQueuePresentKHR(ctx.presentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
		ctx.window->size() != glm::uvec2{swapchain.extent.width, swapchain.extent.height})
		refresh();
	else if (result != VK_SUCCESS)
		VK(result);
}

}

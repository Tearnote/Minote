#include "gfx/engine.hpp"

#include <string_view>
#include <stdexcept>
#include <algorithm>
#include <optional>
#include <cstring>
#include <string>
#include <vector>
#include <limits>
#include <thread>
#include <array>
#include <glm/gtx/transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <GLFW/glfw3.h>
#include <fmt/core.h>
#include "volk/volk.h"
#include "base/zip_view.hpp"
#include "base/types.hpp"
#include "base/math.hpp"
#include "base/time.hpp"
#include "base/log.hpp"
#include "sys/vk/framebuffer.hpp"
#include "sys/vk/descriptor.hpp"
#include "sys/vk/commands.hpp"
#include "sys/vk/pipeline.hpp"
#include "sys/vk/debug.hpp"
#include "sys/vk/base.hpp"
#include "sys/window.hpp"
#include "sys/glfw.hpp"
#include "mesh/block.hpp"
#include "mesh/scene.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;
namespace vk = sys::vk;
namespace ranges = std::ranges;
using namespace std::string_literals;

constexpr auto presentVertSrc = std::to_array<u32>({
#include "spv/present.vert.spv"
});

constexpr auto presentFragSrc = std::to_array<u32>({
#include "spv/present.frag.spv"
});

constexpr auto bloomVertSrc = std::to_array<u32>({
#include "spv/bloom.vert.spv"
});

constexpr auto bloomFragSrc = std::to_array<u32>({
#include "spv/bloom.frag.spv"
});

Engine::Engine(sys::Glfw&, sys::Window& window, std::string_view name, Version appVersion) {
	ctx.init(window, VulkanVersion, name, appVersion);
	swapchain.init(ctx);
	initCommands();
	samplers.init(ctx);
	initImages();
	initFramebuffers();
	initBuffers();
	initPipelines();

	L.info("Vulkan initialized");
}

Engine::~Engine() {
	vkDeviceWaitIdle(ctx.device);

	for (auto& op: delayedOps)
		op.func();

	cleanupPipelines();
	cleanupBuffers();
	cleanupFramebuffers();
	cleanupImages();
	samplers.cleanup(ctx);
	cleanupCommands();
	swapchain.cleanup(ctx);
	ctx.cleanup();

	L.info("Vulkan cleaned up");
}

void Engine::setBackground(glm::vec3 color) {
	world.uniforms.ambientColor = glm::vec4{color, 1.0f};
}

void Engine::setLightSource(glm::vec3 position, glm::vec3 color) {
	world.uniforms.lightPosition = glm::vec4{position, 1.0f};
	world.uniforms.lightColor = glm::vec4{color, 1.0f};
}

void Engine::setCamera(glm::vec3 eye, glm::vec3 center, glm::vec3 up) {
	camera = Camera{
		.eye = eye,
		.center = center,
		.up = up,
	};
}

void Engine::enqueueDraw(ID mesh, ID technique, std::span<Instance const> instances,
	Material material, MaterialData const& materialData) {
	auto const frameIndex = frameCounter % FramesInFlight;
	auto& indirect = techniques.getTechniqueIndirect(technique, frameIndex);
	indirect.enqueue(meshes.getMeshDescriptor(mesh), instances, material, materialData);
}

void Engine::render() {
	// Get the next frame
	auto frameIndex = frameCounter % FramesInFlight;
	auto& frame = frames[frameIndex];
	auto cmdBuf = frame.commandBuffer;

	// Get the next swapchain image
	u32 swapchainImageIndex;
	while (true) {
		auto result = vkAcquireNextImageKHR(ctx.device, swapchain.swapchain, std::numeric_limits<u64>::max(),
			frame.presentSemaphore, nullptr, &swapchainImageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			refresh();
			continue;
		}
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
			VK(result);
		break;
	}

	// Wait until the GPU is free
	VK(vkWaitForFences(ctx.device, 1, &frame.renderFence, true, (1_s).count()));
	VK(vkResetFences(ctx.device, 1, &frame.renderFence));

	// Retrieve the techniques in use
	auto& opaque = techniques.getTechnique("opaque"_id);
	auto& opaqueIndirect = techniques.getTechniqueIndirect("opaque"_id, frameIndex);
	auto& transparentDepthPrepass = techniques.getTechnique("transparent_depth_prepass"_id);
	auto& transparentDepthPrepassIndirect = techniques.getTechniqueIndirect("transparent_depth_prepass"_id, frameIndex);
	auto& transparent = techniques.getTechnique("transparent"_id);
	auto& transparentIndirect = techniques.getTechniqueIndirect("transparent"_id, frameIndex);

	// Prepare and upload draw data to the GPU
	opaqueIndirect.upload(ctx.allocator);
	transparentDepthPrepassIndirect.upload(ctx.allocator);
	transparentIndirect.upload(ctx.allocator);

	world.uniforms.setViewProjection(glm::uvec2{swapchain.extent.width, swapchain.extent.height},
		VerticalFov, NearPlane, FarPlane,
		camera.eye, camera.center, camera.up);
	world.uploadUniforms(ctx.allocator, frameIndex);

	// Start recording commands
	VK(vkResetCommandBuffer(cmdBuf, 0));
	auto cmdBeginInfo = VkCommandBufferBeginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	VK(vkBeginCommandBuffer(cmdBuf, &cmdBeginInfo));

	// Bind world data
	vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, techniques.getPipelineLayout(),
		0, 1, &world.getDescriptorSet(frameIndex), 0, nullptr);

	// Begin drawing objects
	vk::cmdBeginRenderPass(cmdBuf, targets.renderPass, targets.framebuffer, swapchain.extent, std::array{
		vk::clearColor(world.uniforms.ambientColor),
		vk::clearDepth(1.0f),
	});
	vk::cmdSetArea(cmdBuf, swapchain.extent);

	// Opaque object draw
	vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, opaque.pipeline);
	vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
		techniques.getPipelineLayout(), 1, 1, &opaque.getDescriptorSet(frameIndex),
		0, nullptr);

	vkCmdDrawIndirect(cmdBuf, opaqueIndirect.commandBuffer().buffer, 0,
		opaqueIndirect.size(), sizeof(IndirectBuffer::Command));

	// Transparent object draw prepass
	vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, transparentDepthPrepass.pipeline);
	vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
		techniques.getPipelineLayout(), 1, 1, &transparentDepthPrepass.getDescriptorSet(frameIndex),
		0, nullptr);

	vkCmdDrawIndirect(cmdBuf, transparentDepthPrepassIndirect.commandBuffer().buffer, 0,
		transparentDepthPrepassIndirect.size(), sizeof(IndirectBuffer::Command));

	// Transparent object draw
	vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, transparent.pipeline);
	vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
		techniques.getPipelineLayout(), 1, 1, &transparent.getDescriptorSet(frameIndex),
		0, nullptr);

	vkCmdDrawIndirect(cmdBuf, transparentIndirect.commandBuffer().buffer, 0,
		transparentIndirect.size(), sizeof(IndirectBuffer::Command));

	// Finish the object drawing pass
	vkCmdEndRenderPass(cmdBuf);

	// Synchronize the rendered color image
	vk::cmdImageBarrier(cmdBuf, targets.ssColor, VK_IMAGE_ASPECT_COLOR_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// HDR-threshold the color image
	vk::cmdBeginRenderPass(cmdBuf, bloom.downPass, bloom.imageFbs[0], bloom.images[0].size);
	vk::cmdSetArea(cmdBuf, bloom.images[0].size);

	vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, bloom.down);
	vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
		bloom.layout, 1, 1, &bloom.sourceDS, 0, nullptr);
	vkCmdDraw(cmdBuf, 3, 1, 0, 0);
	vkCmdEndRenderPass(cmdBuf);

	// Synchronize the thresholded image
	vk::cmdImageBarrier(cmdBuf, bloom.images[0], VK_IMAGE_ASPECT_COLOR_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Progressively downscale the bloom contents
	for (size_t i = 1; i < Bloom::Depth; i += 1) {
		vk::cmdBeginRenderPass(cmdBuf, bloom.downPass, bloom.imageFbs[i], bloom.images[i].size);
		vk::cmdSetArea(cmdBuf, bloom.images[i].size);
		vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, bloom.down);
		vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
			bloom.layout, 1, 1, &bloom.imageDS[i - 1], 0, nullptr);
		vkCmdDraw(cmdBuf, 3, 1, 0, 1);
		vkCmdEndRenderPass(cmdBuf);

		vk::cmdImageBarrier(cmdBuf, bloom.images[i], VK_IMAGE_ASPECT_COLOR_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	// Progressively upscale the bloom contents
	for (size_t i = Bloom::Depth - 2; i < Bloom::Depth; i -= 1) {
		vk::cmdImageBarrier(cmdBuf, bloom.images[i], VK_IMAGE_ASPECT_COLOR_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0, 0,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		vk::cmdBeginRenderPass(cmdBuf, bloom.upPass, bloom.imageFbs[i], bloom.images[i].size);
		vk::cmdSetArea(cmdBuf, bloom.images[i].size);
		vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, bloom.up);
		vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
			bloom.layout, 1, 1, &bloom.imageDS[i + 1], 0, nullptr);
		vkCmdDraw(cmdBuf, 3, 1, 0, 2);
		vkCmdEndRenderPass(cmdBuf);

		vk::cmdImageBarrier(cmdBuf, bloom.images[i], VK_IMAGE_ASPECT_COLOR_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	// Synchronize the rendered color image
	vk::cmdImageBarrier(cmdBuf, targets.ssColor, VK_IMAGE_ASPECT_COLOR_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0, 0,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	// Apply bloom to rendered color image
	vk::cmdBeginRenderPass(cmdBuf, bloom.upPass, bloom.targetFb, targets.ssColor.size);
	vk::cmdSetArea(cmdBuf, swapchain.extent);
	vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, bloom.up);
	vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
		bloom.layout, 1, 1, &bloom.imageDS[0], 0, nullptr);
	vkCmdDraw(cmdBuf, 3, 1, 0, 2);
	vkCmdEndRenderPass(cmdBuf);

	// Synchronize the rendered color image
	vk::cmdImageBarrier(cmdBuf, targets.ssColor, VK_IMAGE_ASPECT_COLOR_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Blit the image to screen
	vk::cmdBeginRenderPass(cmdBuf, present.renderPass, present.framebuffer[swapchainImageIndex], swapchain.extent);
	vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, present.pipeline);
	vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
		present.layout, 1, 1, &present.descriptorSet, 0, nullptr);
	vkCmdDraw(cmdBuf, 3, 1, 0, 0);
	vkCmdEndRenderPass(cmdBuf);

	// Finish recording commands
	VK(vkEndCommandBuffer(cmdBuf));

	// Submit commands to the queue
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	auto submitInfo = VkSubmitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &frame.presentSemaphore,
		.pWaitDstStageMask = &waitStage,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmdBuf,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &frame.renderSemaphore,
	};
	VK(vkQueueSubmit(ctx.graphicsQueue, 1, &submitInfo, frame.renderFence));

	// Present the rendered image
	auto presentInfo = VkPresentInfoKHR{
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

	// Run delayed ops
	auto newsize = ranges::remove_if(delayedOps, [this](auto& op) {
		if (op.deadline == frameCounter) {
			op.func();
			return true;
		} else {
			return false;
		}
	});
	delayedOps.erase(newsize.begin(), newsize.end());

	// Advance, cleanup
	frameCounter += 1;
	opaqueIndirect.reset();
	transparentDepthPrepassIndirect.reset();
	transparentIndirect.reset();
}

void Engine::initCommands() {
	// Create the descriptor pool
	auto const descriptorPoolSizes = std::to_array<VkDescriptorPoolSize>({
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 64 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 64 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 64 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 64 },
	});
	auto descriptorPoolCI = VkDescriptorPoolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = 256,
		.poolSizeCount = static_cast<u32>(descriptorPoolSizes.size()),
		.pPoolSizes = descriptorPoolSizes.data(),
	};
	VK(vkCreateDescriptorPool(ctx.device, &descriptorPoolCI, nullptr, &descriptorPool));
	vk::setDebugName(ctx.device, descriptorPool, "descriptorPool");

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
		vk::setDebugName(ctx.device, frame.commandPool, fmt::format("frames[{}].commandPool", &frame - &frames[0]));
		commandBufferAI.commandPool = frame.commandPool;
		VK(vkAllocateCommandBuffers(ctx.device, &commandBufferAI, &frame.commandBuffer));
		vk::setDebugName(ctx.device, frame.commandBuffer, fmt::format("frames[{}].commandBuffer", &frame - &frames[0]));
		VK(vkCreateFence(ctx.device, &fenceCI, nullptr, &frame.renderFence));
		vk::setDebugName(ctx.device, frame.renderFence, fmt::format("frames[{}].renderFence", &frame - &frames[0]));
		VK(vkCreateSemaphore(ctx.device, &semaphoreCI, nullptr, &frame.renderSemaphore));
		vk::setDebugName(ctx.device, frame.renderSemaphore, fmt::format("frames[{}].renderSemaphore", &frame - &frames[0]));
		VK(vkCreateSemaphore(ctx.device, &semaphoreCI, nullptr, &frame.presentSemaphore));
		vk::setDebugName(ctx.device, frame.presentSemaphore, fmt::format("frames[{}].presentSemaphore", &frame - &frames[0]));
	}

	// Create the transfer queue
	commandPoolCI.queueFamilyIndex = ctx.transferQueueFamilyIndex;
	VK(vkCreateCommandPool(ctx.device, &commandPoolCI, nullptr, &transferCommandPool));
	vk::setDebugName(ctx.device, transferCommandPool, "transferCommandPool");
	fenceCI.flags = 0;
	VK(vkCreateFence(ctx.device, &fenceCI, nullptr, &transfersFinished));
	vk::setDebugName(ctx.device, transfersFinished, "transfersFinished");
}

void Engine::cleanupCommands() {
	vkDestroyFence(ctx.device, transfersFinished, nullptr);
	vkDestroyCommandPool(ctx.device, transferCommandPool, nullptr);
	for (auto& frame: frames) {
		vkDestroySemaphore(ctx.device, frame.presentSemaphore, nullptr);
		vkDestroySemaphore(ctx.device, frame.renderSemaphore, nullptr);
		vkDestroyFence(ctx.device, frame.renderFence, nullptr);
		vkDestroyCommandPool(ctx.device, frame.commandPool, nullptr);
	}
	vkDestroyDescriptorPool(ctx.device, descriptorPool, nullptr);
}

void Engine::initImages() {
	auto const supportedSampleCount = ctx.deviceProperties.limits.framebufferColorSampleCounts & ctx.deviceProperties.limits.framebufferDepthSampleCounts;
	auto const selectedSampleCount = [=]() {
		if (SampleCount & supportedSampleCount) {
			return SampleCount;
		} else {
			L.warn("Requested antialiasing mode MSAA {}x not supported; defaulting to MSAA 2x",
				SampleCount);
			return VK_SAMPLE_COUNT_2_BIT;
		}
	}();
	targets.init(ctx.device, ctx.allocator, swapchain.extent, ColorFormat, DepthFormat, selectedSampleCount);
	createBloomImages();
}

void Engine::cleanupImages() {
	destroyBloomImages(bloom);
	targets.cleanup(ctx.device, ctx.allocator);
}

void Engine::initFramebuffers() {
	createPresentFbs();
	createBloomFbs();
}

void Engine::cleanupFramebuffers() {
	destroyBloomFbs(bloom);
	destroyPresentFbs(present);
}

void Engine::initBuffers() {
	// Begin GPU transfers
	auto commandBufferAI = VkCommandBufferAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = transferCommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	VkCommandBuffer transferCommandBuffer;
	VK(vkAllocateCommandBuffers(ctx.device, &commandBufferAI, &transferCommandBuffer));
	vk::setDebugName(ctx.device, transferCommandBuffer, "transferCommandBuffer");

	auto commandBufferBI = VkCommandBufferBeginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	VK(vkBeginCommandBuffer(transferCommandBuffer, &commandBufferBI));

	// Upload buffers to GPU
	std::vector<vk::Buffer> stagingBuffers;

	createMeshBuffer(transferCommandBuffer, stagingBuffers);

	// Finish GPU transfers
	VK(vkEndCommandBuffer(transferCommandBuffer));

	auto submitInfo = VkSubmitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &transferCommandBuffer,
	};
	VK(vkQueueSubmit(ctx.transferQueue, 1, &submitInfo, transfersFinished));

	vkWaitForFences(ctx.device, 1, &transfersFinished, true, (1_s).count());
	vkResetFences(ctx.device, 1, &transfersFinished);

	vkResetCommandPool(ctx.device, transferCommandPool, 0);

	for (auto& buffer: stagingBuffers)
		vmaDestroyBuffer(ctx.allocator, buffer.buffer, buffer.allocation);

	// Create CPU to GPU buffers
	world.create(ctx.device, ctx.allocator, descriptorPool, meshes);
	world.setDebugName(ctx.device);
}

void Engine::cleanupBuffers() {
	world.destroy(ctx.device, ctx.allocator);
	destroyMeshBuffer();
}

void Engine::initPipelines() {
	createPresentPipeline();
	createPresentPipelineDS();
	techniques.create(ctx.device, world.getDescriptorSetLayout());
	techniques.addTechnique("opaque"_id, ctx.device, ctx.allocator, targets.renderPass,
		descriptorPool, world.getDescriptorSets(),
		vk::makePipelineRasterizationStateCI(VK_POLYGON_MODE_FILL, true),
		vk::makePipelineColorBlendAttachmentState(vk::BlendingMode::None),
		vk::makePipelineDepthStencilStateCI(true, true, VK_COMPARE_OP_LESS_OR_EQUAL),
		vk::makePipelineMultisampleStateCI(targets.msColor.samples));
	techniques.setTechniqueDebugName(ctx.device, "opaque"_id, "opaque");
	techniques.addTechnique("transparent_depth_prepass"_id, ctx.device, ctx.allocator, targets.renderPass,
		descriptorPool, world.getDescriptorSets(),
		vk::makePipelineRasterizationStateCI(VK_POLYGON_MODE_FILL, true),
		vk::makePipelineColorBlendAttachmentState(vk::BlendingMode::None, false),
		vk::makePipelineDepthStencilStateCI(true, true, VK_COMPARE_OP_LESS_OR_EQUAL),
		vk::makePipelineMultisampleStateCI(targets.msColor.samples));
	techniques.setTechniqueDebugName(ctx.device, "transparent_depth_prepass"_id, "transparent_depth_prepass");
	techniques.addTechnique("transparent"_id, ctx.device, ctx.allocator, targets.renderPass,
		descriptorPool, world.getDescriptorSets(),
		vk::makePipelineRasterizationStateCI(VK_POLYGON_MODE_FILL, true),
		vk::makePipelineColorBlendAttachmentState(vk::BlendingMode::Normal),
		vk::makePipelineDepthStencilStateCI(true, false, VK_COMPARE_OP_LESS_OR_EQUAL),
		vk::makePipelineMultisampleStateCI(targets.msColor.samples));
	techniques.setTechniqueDebugName(ctx.device, "transparent"_id, "transparent");
	createBloomPipelines();
	createBloomPipelineDS();
}

void Engine::cleanupPipelines() {
	destroyBloomPipelineDS(bloom);
	destroyBloomPipelines();
	techniques.destroy(ctx.device, ctx.allocator);
	destroyPresentPipelineDS(present);
	destroyPresentPipeline();
}

void Engine::refresh() {
	ctx.refreshSurface();
	auto const sampleCount = targets.msColor.samples;

	// Queue up outdated objects for destruction
	delayedOps.emplace_back(DelayedOp{
		.deadline = frameCounter + FramesInFlight,
		.func = [this, swapchain=swapchain, targets=targets, present=present, bloom=bloom]() mutable {
			destroyPresentPipelineDS(present);
			destroyBloomPipelineDS(bloom);
			destroyBloomFbs(bloom);
			destroyPresentFbs(present);
			destroyBloomImages(bloom);
			targets.refreshCleanup(ctx.device, ctx.allocator);
			swapchain.cleanup(ctx);
		},
	});

	// Create new objects
	auto oldSwapchain = swapchain.swapchain;
	swapchain = {};
	targets = {};
	swapchain.init(ctx, oldSwapchain);
	targets.refreshInit(ctx.device, ctx.allocator, swapchain.extent, ColorFormat, DepthFormat, sampleCount);
	createBloomImages();
	createPresentFbs();
	createBloomFbs();
	createBloomPipelineDS();
	createPresentPipelineDS();
}

void Engine::createPresentFbs() {
	// Create the present render pass
	present.renderPass = vk::createRenderPass(ctx.device, std::array{
		vk::Attachment{ // Source
			.type = vk::Attachment::Type::Input,
			.image = targets.ssColor,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.layoutBefore = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		},
		vk::Attachment{ // Present
			.type = vk::Attachment::Type::Color,
			.image = swapchain.color[0],
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.layoutDuring = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.layoutAfter = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		},
	});
	vk::setDebugName(ctx.device, present.renderPass, "present.renderPass");

	// Create the present framebuffers
	present.framebuffer.resize(swapchain.color.size());
	for (auto[fb, image]: zip_view{present.framebuffer, swapchain.color}) {
		fb = vk::createFramebuffer(ctx.device, present.renderPass, std::array{
			targets.ssColor,
			image,
		});
		vk::setDebugName(ctx.device, fb, fmt::format("present.framebuffer[{}]", &fb - &present.framebuffer[0]));
	}
}

void Engine::destroyPresentFbs(Present& p) {
	for (auto& fb: p.framebuffer)
		vkDestroyFramebuffer(ctx.device, fb, nullptr);
	vkDestroyRenderPass(ctx.device, p.renderPass, nullptr);
}

void Engine::createPresentPipeline() {
	present.descriptorSetLayout = vk::createDescriptorSetLayout(ctx.device, std::array{
		vk::Descriptor{
			.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
			.stages = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
	});
	vk::setDebugName(ctx.device, present.descriptorSetLayout, "present.descriptorSetLayout");
	present.shader = vk::createShader(ctx.device, presentVertSrc, presentFragSrc);
	vk::setDebugName(ctx.device, present.shader, "present.shader");

	present.layout = vk::createPipelineLayout(ctx.device, std::array{
		world.getDescriptorSetLayout(),
		present.descriptorSetLayout,
	});
	vk::setDebugName(ctx.device, present.layout, "present.layout");
	present.pipeline = vk::PipelineBuilder{
		.shader = present.shader,
		.vertexInputStateCI = vk::makePipelineVertexInputStateCI(),
		.inputAssemblyStateCI = vk::makePipelineInputAssemblyStateCI(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST),
		.rasterizationStateCI = vk::makePipelineRasterizationStateCI(VK_POLYGON_MODE_FILL, false),
		.colorBlendAttachmentState = vk::makePipelineColorBlendAttachmentState(vk::BlendingMode::None),
		.depthStencilStateCI = vk::makePipelineDepthStencilStateCI(false, false, VK_COMPARE_OP_ALWAYS),
		.multisampleStateCI = vk::makePipelineMultisampleStateCI(),
		.layout = present.layout,
	}.build(ctx.device, present.renderPass);
	vk::setDebugName(ctx.device, present.pipeline, "present.pipeline");
}

void Engine::destroyPresentPipeline() {
	vkDestroyPipeline(ctx.device, present.pipeline, nullptr);
	vkDestroyPipelineLayout(ctx.device, present.layout, nullptr);
	vk::destroyShader(ctx.device, present.shader);
	vkDestroyDescriptorSetLayout(ctx.device, present.descriptorSetLayout, nullptr);
}

void Engine::createPresentPipelineDS() {
	present.descriptorSet = vk::allocateDescriptorSet(ctx.device, descriptorPool, present.descriptorSetLayout);
	vk::setDebugName(ctx.device, present.descriptorSet, "present.descriptorSet");
	vk::updateDescriptorSets(ctx.device, std::array{
		vk::makeDescriptorSetImageWrite(present.descriptorSet, 0, targets.ssColor,
			VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
	});
}

void Engine::destroyPresentPipelineDS(Present& p) {
	vkFreeDescriptorSets(ctx.device, descriptorPool, 1, &p.descriptorSet);
}

void Engine::createMeshBuffer(VkCommandBuffer cmdBuf, std::vector<sys::vk::Buffer>& staging) {
	meshes.addMesh("block"_id, generateNormals(mesh::Block));
	meshes.addMesh("scene"_id, generateNormals(mesh::Scene));
	meshes.upload(ctx.allocator, cmdBuf, staging.emplace_back());
	meshes.setDebugName(ctx.device);
}

void Engine::destroyMeshBuffer() {
	meshes.destroy(ctx.allocator);
}

void Engine::createBloomImages() {
	auto extent = swapchain.extent;
	for (auto& image: bloom.images) {
		image = vk::createImage(ctx.device, ctx.allocator, ColorFormat, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, extent);
		vk::setDebugName(ctx.device, image, fmt::format("bloom.images[{}]", &image - &bloom.images[0]));
		extent.width = std::max(1u, extent.width >> 1);
		extent.height = std::max(1u, extent.height >> 1);
	}
}

void Engine::destroyBloomImages(Bloom& b) {
	for (auto& image: b.images)
		vk::destroyImage(ctx.device, ctx.allocator, image);
}

void Engine::createBloomFbs() {
	bloom.downPass = vk::createRenderPass(ctx.device, std::array{
		vk::Attachment{
			.type = vk::Attachment::Type::Color,
			.image = bloom.images[0],
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.layoutDuring = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		},
	});
	vk::setDebugName(ctx.device, bloom.downPass, "bloom.downPass");
	bloom.upPass = vk::createRenderPass(ctx.device, std::array{
		vk::Attachment{
			.type = vk::Attachment::Type::Color,
			.image = bloom.images[0],
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.layoutBefore = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		},
	});
	vk::setDebugName(ctx.device, bloom.upPass, "bloom.upPass");

	bloom.targetFb = vk::createFramebuffer(ctx.device, bloom.downPass, std::array{targets.ssColor});
	vk::setDebugName(ctx.device, bloom.targetFb, "bloom.targetFb");
	for (auto[fb, image]: zip_view{bloom.imageFbs, bloom.images}) {
		fb = vk::createFramebuffer(ctx.device, bloom.downPass, std::array{image});
		vk::setDebugName(ctx.device, fb, fmt::format("bloom.imageFbs[{}]", &fb - &bloom.imageFbs[0]));
	}
}

void Engine::destroyBloomFbs(Bloom& b) {
	vkDestroyFramebuffer(ctx.device, b.targetFb, nullptr);
	for (auto fb: b.imageFbs)
		vkDestroyFramebuffer(ctx.device, fb, nullptr);
	vkDestroyRenderPass(ctx.device, b.upPass, nullptr);
	vkDestroyRenderPass(ctx.device, b.downPass, nullptr);
}

void Engine::createBloomPipelines() {
	bloom.descriptorSetLayout = vk::createDescriptorSetLayout(ctx.device, std::array{
		vk::Descriptor{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.stages = VK_SHADER_STAGE_FRAGMENT_BIT,
			.sampler = samplers.linear,
		},
	});
	vk::setDebugName(ctx.device, bloom.descriptorSetLayout, "bloom.descriptorSetLayout");
	bloom.shader = vk::createShader(ctx.device, bloomVertSrc, bloomFragSrc);
	vk::setDebugName(ctx.device, bloom.shader, "bloom.shader");

	bloom.layout = vk::createPipelineLayout(ctx.device, std::array{
		world.getDescriptorSetLayout(),
		bloom.descriptorSetLayout,
	});
	vk::setDebugName(ctx.device, bloom.layout, "bloom.layout");
	auto builder = vk::PipelineBuilder{
		.shader = bloom.shader,
		.vertexInputStateCI = vk::makePipelineVertexInputStateCI(),
		.inputAssemblyStateCI = vk::makePipelineInputAssemblyStateCI(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST),
		.rasterizationStateCI = vk::makePipelineRasterizationStateCI(VK_POLYGON_MODE_FILL, false),
		.colorBlendAttachmentState = vk::makePipelineColorBlendAttachmentState(vk::BlendingMode::None),
		.depthStencilStateCI = vk::makePipelineDepthStencilStateCI(false, false, VK_COMPARE_OP_ALWAYS),
		.multisampleStateCI = vk::makePipelineMultisampleStateCI(),
		.layout = bloom.layout,
	};
	bloom.down = builder.build(ctx.device, bloom.downPass);
	vk::setDebugName(ctx.device, bloom.down, "bloom.down");
	builder.colorBlendAttachmentState = vk::makePipelineColorBlendAttachmentState(vk::BlendingMode::Add);
	bloom.up = builder.build(ctx.device, bloom.upPass);
	vk::setDebugName(ctx.device, bloom.up, "bloom.up");
}

void Engine::destroyBloomPipelines() {
	vkDestroyPipeline(ctx.device, bloom.up, nullptr);
	vkDestroyPipeline(ctx.device, bloom.down, nullptr);
	vkDestroyPipelineLayout(ctx.device, bloom.layout, nullptr);
	vk::destroyShader(ctx.device, bloom.shader);
	vkDestroyDescriptorSetLayout(ctx.device, bloom.descriptorSetLayout, nullptr);
}

void Engine::createBloomPipelineDS() {
	bloom.sourceDS = vk::allocateDescriptorSet(ctx.device, descriptorPool, bloom.descriptorSetLayout);
	vk::setDebugName(ctx.device, bloom.sourceDS, "bloom.sourceDS");
	for (auto& ds: bloom.imageDS) {
		ds = vk::allocateDescriptorSet(ctx.device, descriptorPool, bloom.descriptorSetLayout);
		vk::setDebugName(ctx.device, ds, fmt::format("bloom.imageDS[{}]", &ds - &bloom.imageDS[0]));
	}

	vk::updateDescriptorSets(ctx.device, std::array{
		vk::makeDescriptorSetImageWrite(bloom.sourceDS, 0, targets.ssColor,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
	});
	for (auto[ds, image]: zip_view{bloom.imageDS, bloom.images}) {
		vk::updateDescriptorSets(ctx.device, std::array{
			vk::makeDescriptorSetImageWrite(ds, 0, image,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
		});
	}
}

void Engine::destroyBloomPipelineDS(Bloom& b) {
	for (auto& ds: b.imageDS)
		vkFreeDescriptorSets(ctx.device, descriptorPool, 1, &ds);
	vkFreeDescriptorSets(ctx.device, descriptorPool, 1, &b.sourceDS);
}

}

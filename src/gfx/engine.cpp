#include "gfx/engine.hpp"

#include <algorithm>
#include <string>
#include <vector>
#include <array>
#include <glm/vec4.hpp>
#include "VulkanMemoryAllocator/vma.h"
#include "volk/volk.h"
#include "base/types.hpp"
#include "base/log.hpp"
#include "sys/vk/framebuffer.hpp"
#include "sys/vk/commands.hpp"
#include "sys/vk/pipeline.hpp"
#include "mesh/block.hpp"
#include "mesh/scene.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;
namespace vk = sys::vk;
namespace ranges = std::ranges;

Engine::Engine(sys::Glfw&, sys::Window& window, std::string_view name, Version appVersion) {
	// Create essential objects
	ctx.init(window, VulkanVersion, name, appVersion);
	swapchain.init(ctx);
	commands.init(ctx);

	// Create rendering infrastructure
	std::vector<vk::Buffer> stagingBuffers;
	commands.transfer(ctx, [this, &stagingBuffers](VkCommandBuffer cmdBuf) {
		meshes.addMesh("block"_id, generateNormals(mesh::Block));
		meshes.addMesh("scene_base"_id, generateNormals(mesh::SceneBase));
		meshes.addMesh("scene_body"_id, generateNormals(mesh::SceneBody));
		meshes.addMesh("scene_top"_id, generateNormals(mesh::SceneTop));
		meshes.addMesh("scene_guide"_id, generateNormals(mesh::SceneGuide));
		meshes.upload(ctx, cmdBuf, stagingBuffers.emplace_back());
	});
	for (auto& buffer: stagingBuffers)
		vmaDestroyBuffer(ctx.allocator, buffer.buffer, buffer.allocation);

	samplers.init(ctx);
	world.init(ctx, meshes);
	targets.init(ctx, swapchain.extent, ColorFormat, DepthFormat);

	// Create the pipeline phases
	techniques.init(ctx, world.getDescriptorSetLayout());
	techniques.addTechnique(ctx, "opaque"_id, targets.renderPass,
		world.getDescriptorSets(),
		vk::makePipelineRasterizationStateCI(VK_POLYGON_MODE_FILL, true),
		vk::makePipelineColorBlendAttachmentState(vk::BlendingMode::None),
		vk::makePipelineDepthStencilStateCI(true, true, VK_COMPARE_OP_LESS_OR_EQUAL));
	techniques.setTechniqueDebugName(ctx, "opaque"_id, "opaque");
	techniques.addTechnique(ctx, "transparent_depth_prepass"_id, targets.renderPass,
		world.getDescriptorSets(),
		vk::makePipelineRasterizationStateCI(VK_POLYGON_MODE_FILL, false),
		vk::makePipelineColorBlendAttachmentState(vk::BlendingMode::None, false),
		vk::makePipelineDepthStencilStateCI(true, true, VK_COMPARE_OP_LESS_OR_EQUAL));
	techniques.setTechniqueDebugName(ctx, "transparent_depth_prepass"_id, "transparent_depth_prepass");
	techniques.addTechnique(ctx, "transparent"_id, targets.renderPass,
		world.getDescriptorSets(),
		vk::makePipelineRasterizationStateCI(VK_POLYGON_MODE_FILL, false),
		vk::makePipelineColorBlendAttachmentState(vk::BlendingMode::Normal),
		vk::makePipelineDepthStencilStateCI(true, false, VK_COMPARE_OP_LESS_OR_EQUAL));
	techniques.setTechniqueDebugName(ctx, "transparent"_id, "transparent");

	bloom.init(ctx, samplers, world, targets.ssColor, ColorFormat);
	present.init(ctx, world, targets.ssColor, swapchain);

	L.info("Vulkan engine initialized");
}

Engine::~Engine() {
	vkDeviceWaitIdle(ctx.device);

	for (auto& op: delayedOps)
		op.func();

	techniques.cleanup(ctx);
	present.cleanup(ctx);
	bloom.cleanup(ctx);
	targets.cleanup(ctx);

	world.cleanup(ctx);
	meshes.cleanup(ctx);
	samplers.cleanup(ctx);
	commands.cleanup(ctx);
	swapchain.cleanup(ctx);
	ctx.cleanup();

	L.info("Vulkan engine cleaned up");
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
	auto& indirect = techniques.getTechnique(technique).indirect[frameIndex];
	indirect.enqueue(meshes.getMeshDescriptor(mesh), instances, material, materialData);
}

void Engine::render() {
	commands.render(ctx, swapchain, frameCounter,
		[this] { refresh(); },
		[this](Commands::Frame& frame, u32 frameIndex, u32 swapchainImageIndex) {
			auto cmdBuf = frame.commandBuffer;

			// Retrieve the techniques in use
			auto& opaque = techniques.getTechnique("opaque"_id);
			auto& opaqueIndirect = opaque.indirect[frameIndex];

			// Prepare and upload draw data to the GPU
			opaqueIndirect.upload(ctx);

			world.uniforms.setViewProjection(glm::uvec2{swapchain.extent.width, swapchain.extent.height},
				VerticalFov, NearPlane, FarPlane,
				camera.eye, camera.center, camera.up);
			world.uploadUniforms(ctx, frameIndex);

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
			for (auto i: nrange(1_zu, Bloom::Depth)) {
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
			for (auto i: rnrange_inc(Bloom::Depth - 2, 0_zu, 1_zu)) {
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

			// Cleanup
			opaqueIndirect.reset();

		});

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

	// Advance
	frameCounter += 1;
}

void Engine::refresh() {
	ctx.refreshSurface();

	// Queue up outdated objects for destruction
	delayedOps.emplace_back(DelayedOp{
		.deadline = frameCounter + FramesInFlight,
		.func = [this, swapchain=swapchain, targets=targets, present=present, bloom=bloom]() mutable {
			present.refreshCleanup(ctx);
			bloom.refreshCleanup(ctx);
			targets.refreshCleanup(ctx);
			swapchain.cleanup(ctx);
		},
	});

	// Create new objects
	auto oldSwapchain = swapchain.swapchain;
	swapchain = {};
	targets = {};
	swapchain.init(ctx, oldSwapchain);
	targets.refreshInit(ctx, swapchain.extent, ColorFormat, DepthFormat);
	bloom.refreshInit(ctx, targets.ssColor, ColorFormat);
	present.refreshInit(ctx, targets.ssColor, swapchain);
}

}

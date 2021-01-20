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

static constexpr auto versionToCode(Version version) -> u32 {
	return (std::get<0>(version) << 22) + (std::get<1>(version) << 12) + std::get<2>(version);
}

static constexpr auto codeToVersion(u32 code) -> Version {
	return {
		code >> 22,
		(code >> 12) & ((1 << 10) - 1),
		code & ((1 << 12) - 1)
	};
}

Engine::Engine(sys::Glfw&, sys::Window& _window, std::string_view _name, Version appVersion):
	name{_name}, window{_window}, frameCounter{0} {
	initInstance(appVersion);
	initPhysicalDevice();
	initDevice();
	initCommands();
	initSamplers();
	initSwapchain();
	initImages();
	initFramebuffers();
	initBuffers();
	initPipelines();

	L.info("Vulkan initialized");
}

Engine::~Engine() {
	if (device) {
		vkDeviceWaitIdle(device);

		for (auto& op: delayedOps)
			op.func();

		cleanupPipelines();
		cleanupBuffers();
		cleanupFramebuffers();
		cleanupImages();
		cleanupSwapchain();
		cleanupSamplers();
		cleanupCommands();
		cleanupDevice();
	}
	if (instance) {
		cleanupInstance();
	}

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
		auto result = vkAcquireNextImageKHR(device, swapchain.swapchain, std::numeric_limits<u64>::max(),
			frame.presentSemaphore, nullptr, &swapchainImageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapchain();
			continue;
		}
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
			VK(result);
		break;
	}

	// Wait until the GPU is free
	VK(vkWaitForFences(device, 1, &frame.renderFence, true, (1_s).count()));
	VK(vkResetFences(device, 1, &frame.renderFence));

	// Retrieve the techniques in use
	auto& opaque = techniques.getTechnique("opaque"_id);
	auto& opaqueIndirect = techniques.getTechniqueIndirect("opaque"_id, frameIndex);
	auto& transparentDepthPrepass = techniques.getTechnique("transparent_depth_prepass"_id);
	auto& transparentDepthPrepassIndirect = techniques.getTechniqueIndirect("transparent_depth_prepass"_id, frameIndex);
	auto& transparent = techniques.getTechnique("transparent"_id);
	auto& transparentIndirect = techniques.getTechniqueIndirect("transparent"_id, frameIndex);

	// Prepare and upload draw data to the GPU
	opaqueIndirect.upload(allocator);
	transparentDepthPrepassIndirect.upload(allocator);
	transparentIndirect.upload(allocator);

	world.uniforms.setViewProjection(glm::uvec2{swapchain.extent.width, swapchain.extent.height},
		VerticalFov, NearPlane, FarPlane,
		camera.eye, camera.center, camera.up);
	world.uploadUniforms(allocator, frameIndex);

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
	VK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, frame.renderFence));

	// Present the rendered image
	auto presentInfo = VkPresentInfoKHR{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &frame.renderSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &swapchain.swapchain,
		.pImageIndices = &swapchainImageIndex,
	};
	auto result = vkQueuePresentKHR(presentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
		window.size() != glm::uvec2{swapchain.extent.width, swapchain.extent.height})
		recreateSwapchain();
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

void Engine::initInstance(Version appVersion) {
	if (!glfwVulkanSupported())
		throw std::runtime_error("Vulkan is not supported by your system");

	// Check if the required Vulkan version is available
	volkInitializeCustom(&glfwGetInstanceProcAddress);
	L.info("Requesting Vulkan version {}", codeToVersion(VulkanVersion));
	auto const vkVersionCode = volkGetInstanceVersion();
	L.info("Vulkan version {} found", codeToVersion(vkVersionCode));
	if (vkVersionCode < VulkanVersion)
		throw std::runtime_error{"Incompatible Vulkan version"};

	// Fill in application info
	auto const appVersionCode = versionToCode(appVersion);
	auto appInfo = VkApplicationInfo{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = name.c_str(),
		.applicationVersion = appVersionCode,
		.pEngineName = "No Engine",
		.engineVersion = appVersionCode,
		.apiVersion = VulkanVersion,
	};

	// Enumerate required extensions
	u32 glfwExtensionCount;
	char const** const glfwExtensions{glfwGetRequiredInstanceExtensions(&glfwExtensionCount)};
	ASSERT(glfwExtensions);
	instanceExtensions.assign(glfwExtensions, glfwExtensions + glfwExtensionCount);
#ifdef VK_VALIDATION
	instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif //VK_VALIDATION
	L.debug("Requesting {} Vulkan instance extensions:", instanceExtensions.size());
	for (char const* e: instanceExtensions)
		L.debug("  {}", e);

	// Get available extensions
	u32 extensionCount;
	VK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));
	auto extensions = std::vector<VkExtensionProperties>(extensionCount);
	VK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()));
	L.debug("Found {} Vulkan instance extensions:", extensions.size());
	for (auto const& e: extensions)
		L.debug("  {}", e.extensionName);

	// Check if all required extensions are available
	for (char const* req: instanceExtensions) {
		auto const result = ranges::find_if(extensions, [=](auto const& ext) {
			return std::strcmp(req, ext.extensionName) == 0;
		});
		if (result == extensions.end())
			throw std::runtime_error{fmt::format("Required Vulkan extension {} is not supported", req)};
	}

#ifdef VK_VALIDATION
	// Enumerate required layers
	instanceLayers = {"VK_LAYER_KHRONOS_validation"};
	L.debug("Requesting {} Vulkan layer{}:",
		instanceLayers.size(), instanceLayers.size() > 1? "s" : "");
	for (char const* l: instanceLayers)
		L.debug("  {}", l);

	// Get available layers
	u32 layerCount;
	VK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));
	auto layers = std::vector<VkLayerProperties>(layerCount);
	VK(vkEnumerateInstanceLayerProperties(&layerCount, layers.data()));
	L.debug("Found {} Vulkan layers:", layers.size());
	for (auto const& l: layers)
		L.debug("  {}", l.layerName);

	// Check if all required layers are available
	for (char const* req: instanceLayers) {
		auto const result = ranges::find_if(layers, [=](auto const& l) {
			return std::strcmp(req, l.layerName) == 0;
		});
		if (result == layers.end())
			throw std::runtime_error{fmt::format("Required Vulkan layer {} is not supported", req)};
	}

	// Prepare the debug messenger
	auto debugCI = VkDebugUtilsMessengerCreateInfoEXT{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity = /* VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | */
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = &debugCallback,
	};
#endif //VK_VALIDATION

	// Create the Vulkan instance
	auto instanceCI = VkInstanceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#ifdef VK_VALIDATION
		.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCI),
#endif //VK_VALIDATION
		.pApplicationInfo = &appInfo,
#ifdef VK_VALIDATION
		.enabledLayerCount = static_cast<u32>(instanceLayers.size()),
		.ppEnabledLayerNames = instanceLayers.data(),
#endif //VK_VALIDATION
		.enabledExtensionCount = static_cast<u32>(instanceExtensions.size()),
		.ppEnabledExtensionNames = instanceExtensions.data(),
	};
	VK(vkCreateInstance(&instanceCI, nullptr, &instance));
	volkLoadInstanceOnly(instance);
#ifdef VK_VALIDATION
	VK(vkCreateDebugUtilsMessengerEXT(instance, &debugCI, nullptr, &debugMessenger));
#endif //VK_VALIDATION
	VK(glfwCreateWindowSurface(instance, window.handle(), nullptr, &surface));

	L.debug("Vulkan instance created");
}

void Engine::cleanupInstance() {
	vkDestroySurfaceKHR(instance, surface, nullptr);
#ifdef VK_VALIDATION
	vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
#endif //VK_VALIDATION
	vkDestroyInstance(instance, nullptr);
}

void Engine::initPhysicalDevice() {
	// Enumerate required device extensions
	deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
	L.debug("Requesting {} Vulkan device extension{}:", deviceExtensions.size(),
		deviceExtensions.size() > 1? "s" : "");
	for (char const* e: deviceExtensions)
		L.debug("  {}", e);

	// Get information on all available devices
	u32 deviceCount;
	VK(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
	if (!deviceCount)
		throw std::runtime_error{"Failed to find any GPUs with Vulkan support"};
	auto physicalDevices = std::vector<VkPhysicalDevice>(deviceCount);
	auto physicalDevicesProperties = std::vector<VkPhysicalDeviceProperties>(deviceCount);
	auto physicalDevicesFeatures = std::vector<VkPhysicalDeviceFeatures>(deviceCount);
	auto physicalDevicesExtensions = std::vector<std::vector<VkExtensionProperties>>(deviceCount);
	auto physicalDevicesGraphicsQueueIndex = std::vector<std::optional<u32>>(deviceCount);
	auto physicalDevicesPresentQueueIndex = std::vector<std::optional<u32>>(deviceCount);
	auto physicalDevicesTransferQueueIndex = std::vector<std::optional<u32>>(deviceCount);
	auto physicalDevicesScore = std::vector<i32>(deviceCount);
	// The following three can only be queried after confirming that device extensions are available
	auto physicalDevicesSurfaceCapabilities = std::vector<VkSurfaceCapabilitiesKHR>(deviceCount);
	auto physicalDevicesSurfaceFormats = std::vector<std::vector<VkSurfaceFormatKHR>>(deviceCount);
	auto physicalDevicesSurfacePresentModes = std::vector<std::vector<VkPresentModeKHR>>(deviceCount);
	VK(vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data()));
	for (auto[dev, prop, feat, ext, gfxqueue, presqueue, tranqueue]:
		zip_view{physicalDevices, physicalDevicesProperties, physicalDevicesFeatures,
		         physicalDevicesExtensions, physicalDevicesGraphicsQueueIndex,
		         physicalDevicesPresentQueueIndex, physicalDevicesTransferQueueIndex}) {
		vkGetPhysicalDeviceProperties(dev, &prop);
		vkGetPhysicalDeviceFeatures(dev, &feat);

		// Query for device extensions
		u32 deviceExtensionCount;
		vkEnumerateDeviceExtensionProperties(dev, nullptr, &deviceExtensionCount, nullptr);
		ext.resize(deviceExtensionCount);
		vkEnumerateDeviceExtensionProperties(dev, nullptr, &deviceExtensionCount, ext.data());

		// Query for queue families
		u32 queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);
		auto queueFamilies = std::vector<VkQueueFamilyProperties>(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilies.data());

		// Find first queue family that supports graphics
		if (auto const result = ranges::find_if(queueFamilies, [](auto const& family) -> bool {
				return family.queueFlags & VK_QUEUE_GRAPHICS_BIT;
			}); result != queueFamilies.end())
			gfxqueue = ranges::distance(queueFamilies.begin(), result);

		// Find first queue family that supports presentation
		for (u32 familyIndex: ranges::iota_view{0_zu, queueFamilies.size()}) {
			VkBool32 presentSupported;
			VK(vkGetPhysicalDeviceSurfaceSupportKHR(dev, familyIndex, surface,
				&presentSupported));
			if (presentSupported) {
				presqueue = familyIndex;
				break;
			}
		}

		// Check if there exists a dedicated transfer queue
		if (auto const result = ranges::find_if(queueFamilies, [](auto const& family) -> bool {
				return (family.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
					!(family.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
					!(family.queueFlags & VK_QUEUE_COMPUTE_BIT);
			}); result != queueFamilies.end())
			tranqueue = ranges::distance(queueFamilies.begin(), result);
	}

	// Query and rate device suitability
	L.info("Available GPUs:");
	for (auto[dev, prop, feat, ext, score, gfxqueue, presqueue, tranqueue, surfcap, surffmt, surfmode]:
		zip_view{physicalDevices, physicalDevicesProperties, physicalDevicesFeatures,
		         physicalDevicesExtensions, physicalDevicesScore, physicalDevicesGraphicsQueueIndex,
		         physicalDevicesPresentQueueIndex, physicalDevicesTransferQueueIndex,
		         physicalDevicesSurfaceCapabilities, physicalDevicesSurfaceFormats,
		         physicalDevicesSurfacePresentModes}) {
		L.info("  {}", prop.deviceName);

		// Version check
		L.info("    Vulkan version: {}", codeToVersion(prop.apiVersion));
		if (prop.apiVersion < VulkanVersion)
			score -= 0x10'00'00;

		// Device feature check
		L.debug("    Multi-draw indirect: {}", feat.multiDrawIndirect? "Supported" : "Unsupported");
		if (!feat.multiDrawIndirect)
			score -= 0x10'00'00;

		// Device extension check
		L.debug("    Found {} Vulkan device extensions:", ext.size());
		for (auto const& e: ext)
			L.debug("      {}", e.extensionName);

		for (char const* req: deviceExtensions) {
			auto const result = ranges::find_if(ext, [=](auto const& e) {
				return std::strcmp(req, e.extensionName) == 0;
			});
			if (result == ext.end())
				score -= 0x10'00'00;
		}

		// Swapchain query and check
		if (score >= 0) {
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface, &surfcap);

			u32 surfaceFormatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &surfaceFormatCount,
				nullptr);
			surffmt.resize(surfaceFormatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &surfaceFormatCount,
				surffmt.data());

			u32 surfacePresentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &surfacePresentModeCount,
				nullptr);
			surfmode.resize(surfaceFormatCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &surfacePresentModeCount,
				surfmode.data());

			if (surffmt.empty() || surfmode.empty())
				score -= 0x10'00'00;
		}

		// Queue family check
		L.debug("    Graphics queue index: {}",
			gfxqueue? std::to_string(gfxqueue.value()) : "N/A"s);
		L.debug("    Presentation queue index: {}",
			presqueue? std::to_string(presqueue.value()) : "N/A"s);
		L.debug("    Transfer queue index: {}",
			tranqueue? std::to_string(tranqueue.value()) : "N/A"s);
		if (!gfxqueue || !presqueue)
			score -= 0x10'00'00;
		if (tranqueue)
			score += 0x1'00'00;

		// Device type scoring
		char const* typeStr;
		score += 0x2'00'00 * [&] {
			switch (prop.deviceType) {
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: typeStr = "Discrete";
				return 4;
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: typeStr = "Integrated";
				return 3;
			case VK_PHYSICAL_DEVICE_TYPE_CPU: typeStr = "Software";
				return 2;
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: typeStr = "Virtual";
				return 1;
			default: typeStr = "Unknown";
				return 0;
			}
		}();
		L.info("    Device type: {}", typeStr);

		// Max texture size scoring (tiebreaker)
		L.info("    Max 2D texture size: {}", prop.limits.maxImageDimension2D);
		score += prop.limits.maxImageDimension2D;

		L.info("    Awarded score: {}", score);
	}

	// Choose the best device
	auto const bestDeviceIndex = ranges::distance(physicalDevicesScore.begin(),
		ranges::max_element(physicalDevicesScore));
	auto const bestDeviceScore = physicalDevicesScore[bestDeviceIndex];
	if (bestDeviceScore < 0)
		throw std::runtime_error{"Failed to find any suitable GPU"};
	physicalDevice = physicalDevices[bestDeviceIndex];
	deviceProperties = physicalDevicesProperties[bestDeviceIndex];
	graphicsQueueFamilyIndex = physicalDevicesGraphicsQueueIndex[bestDeviceIndex].value();
	presentQueueFamilyIndex = physicalDevicesPresentQueueIndex[bestDeviceIndex].value();
	transferQueueFamilyIndex = physicalDevicesTransferQueueIndex[bestDeviceIndex]?
		physicalDevicesTransferQueueIndex[bestDeviceIndex].value() :
		graphicsQueueFamilyIndex;
	surfaceCapabilities = physicalDevicesSurfaceCapabilities[bestDeviceIndex];
	surfaceFormats = std::move(physicalDevicesSurfaceFormats[bestDeviceIndex]);
	surfacePresentModes = std::move(physicalDevicesSurfacePresentModes[bestDeviceIndex]);
	L.info("Chosen GPU: {} (Score: {})",
		physicalDevicesProperties[bestDeviceIndex].deviceName, bestDeviceScore);
}

void Engine::initDevice() {
	// Enumerate required queues
	std::vector<VkDeviceQueueCreateInfo> queueCIs;
	queueCIs.reserve(3);
	auto const queuePriority = 1.0f;
	auto queueCI = VkDeviceQueueCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = graphicsQueueFamilyIndex,
		.queueCount = 1,
		.pQueuePriorities = &queuePriority,
	};
	queueCIs.push_back(queueCI);
	if (uniquePresentQueue()) {
		queueCI.queueFamilyIndex = presentQueueFamilyIndex;
		queueCIs.push_back(queueCI);
	}
	if (uniqueTransferQueue()) {
		queueCI.queueFamilyIndex = transferQueueFamilyIndex;
		queueCIs.push_back(queueCI);
	}

	// Create logical device
	auto wantedDeviceFeatures = VkPhysicalDeviceFeatures{
		.multiDrawIndirect = VK_TRUE,
	};
	auto deviceCI = VkDeviceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = static_cast<u32>(queueCIs.size()),
		.pQueueCreateInfos = queueCIs.data(),
#ifdef VK_VALIDATION
		.enabledLayerCount = static_cast<u32>(instanceLayers.size()),
		.ppEnabledLayerNames = instanceLayers.data(),
#endif //VK_VALIDATION
		.enabledExtensionCount = static_cast<u32>(deviceExtensions.size()),
		.ppEnabledExtensionNames = deviceExtensions.data(),
		.pEnabledFeatures = &wantedDeviceFeatures,
	};
	VK(vkCreateDevice(physicalDevice, &deviceCI, nullptr, &device));
	volkLoadDevice(device);

	// Retrieve device queues
	vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
	vkGetDeviceQueue(device, presentQueueFamilyIndex, 0, &presentQueue);
	vkGetDeviceQueue(device, transferQueueFamilyIndex, 0, &transferQueue);

	// Create the allocator
	auto allocatorFunctions = VmaVulkanFunctions{
		.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties,
		.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties,
		.vkAllocateMemory = vkAllocateMemory,
		.vkFreeMemory = vkFreeMemory,
		.vkMapMemory = vkMapMemory,
		.vkUnmapMemory = vkUnmapMemory,
		.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges,
		.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges,
		.vkBindBufferMemory = vkBindBufferMemory,
		.vkBindImageMemory = vkBindImageMemory,
		.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements,
		.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements,
		.vkCreateBuffer = vkCreateBuffer,
		.vkDestroyBuffer = vkDestroyBuffer,
		.vkCreateImage = vkCreateImage,
		.vkDestroyImage = vkDestroyImage,
		.vkCmdCopyBuffer = vkCmdCopyBuffer,
		.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2,
		.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2,
		.vkBindBufferMemory2KHR = vkBindBufferMemory2,
		.vkBindImageMemory2KHR = vkBindImageMemory2,
		.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2,
	};
	auto allocatorCI = VmaAllocatorCreateInfo{
		.physicalDevice = physicalDevice,
		.device = device,
		.pVulkanFunctions = &allocatorFunctions,
		.instance = instance,
		.vulkanApiVersion = VulkanVersion,
	};
	VK(vmaCreateAllocator(&allocatorCI, &allocator));

	L.debug("Vulkan device created");
}

void Engine::cleanupDevice() {
	vmaDestroyAllocator(allocator);
	vkDestroyDevice(device, nullptr);
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
	VK(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &descriptorPool));

	// Create the graphics command buffers and sync objects
	auto commandPoolCI = VkCommandPoolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = graphicsQueueFamilyIndex,
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
		VK(vkCreateCommandPool(device, &commandPoolCI, nullptr, &frame.commandPool));
		commandBufferAI.commandPool = frame.commandPool;
		VK(vkAllocateCommandBuffers(device, &commandBufferAI, &frame.commandBuffer));
		VK(vkCreateFence(device, &fenceCI, nullptr, &frame.renderFence));
		VK(vkCreateSemaphore(device, &semaphoreCI, nullptr, &frame.renderSemaphore));
		VK(vkCreateSemaphore(device, &semaphoreCI, nullptr, &frame.presentSemaphore));
	}

	// Create the transfer queue
	commandPoolCI.queueFamilyIndex = transferQueueFamilyIndex;
	VK(vkCreateCommandPool(device, &commandPoolCI, nullptr, &transferCommandPool));
	fenceCI.flags = 0;
	VK(vkCreateFence(device, &fenceCI, nullptr, &transfersFinished));
}

void Engine::cleanupCommands() {
	vkDestroyFence(device, transfersFinished, nullptr);
	vkDestroyCommandPool(device, transferCommandPool, nullptr);
	for (auto& frame: frames) {
		vkDestroySemaphore(device, frame.presentSemaphore, nullptr);
		vkDestroySemaphore(device, frame.renderSemaphore, nullptr);
		vkDestroyFence(device, frame.renderFence, nullptr);
		vkDestroyCommandPool(device, frame.commandPool, nullptr);
	}
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void Engine::initSamplers() {
	auto const samplerCI = VkSamplerCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	};
	VK(vkCreateSampler(device, &samplerCI, nullptr, &linear));
}

void Engine::cleanupSamplers() {
	vkDestroySampler(device, linear, nullptr);
}

void Engine::initSwapchain() {
	createSwapchain();

	L.info("Created a Vulkan swapchain at {}x{} with {} images",
		swapchain.extent.width, swapchain.extent.height, swapchain.color.size());
}

void Engine::cleanupSwapchain() {
	destroySwapchain(swapchain);
}

void Engine::initImages() {
	createTargetImages();
	createBloomImages();
}

void Engine::cleanupImages() {
	destroyBloomImages(bloom);
	destroyTargetImages(targets);
}

void Engine::initFramebuffers() {
	createPresentFbs();
	createTargetFbs();
	createBloomFbs();
}

void Engine::cleanupFramebuffers() {
	destroyBloomFbs(bloom);
	destroyTargetFbs(targets);
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
	VK(vkAllocateCommandBuffers(device, &commandBufferAI, &transferCommandBuffer));

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
	VK(vkQueueSubmit(transferQueue, 1, &submitInfo, transfersFinished));

	vkWaitForFences(device, 1, &transfersFinished, true, (1_s).count());
	vkResetFences(device, 1, &transfersFinished);

	vkResetCommandPool(device, transferCommandPool, 0);

	for (auto& buffer: stagingBuffers)
		vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);

	// Create CPU to GPU buffers
	world.create(device, allocator, descriptorPool, meshes);
}

void Engine::cleanupBuffers() {
	world.destroy(device, allocator);
	destroyMeshBuffer();
}

void Engine::initPipelines() {
	createPresentPipeline();
	createPresentPipelineDS();
	techniques.create(device, world.getDescriptorSetLayout());
	techniques.addTechnique("opaque"_id, device, allocator, targets.renderPass,
		descriptorPool, world.getDescriptorSets(),
		vk::makePipelineRasterizationStateCI(VK_POLYGON_MODE_FILL, true),
		vk::makePipelineColorBlendAttachmentState(vk::BlendingMode::None),
		vk::makePipelineDepthStencilStateCI(true, true, VK_COMPARE_OP_LESS_OR_EQUAL),
		vk::makePipelineMultisampleStateCI(targets.msColor.samples));
	techniques.addTechnique("transparent_depth_prepass"_id, device, allocator, targets.renderPass,
		descriptorPool, world.getDescriptorSets(),
		vk::makePipelineRasterizationStateCI(VK_POLYGON_MODE_FILL, true),
		vk::makePipelineColorBlendAttachmentState(vk::BlendingMode::None, false),
		vk::makePipelineDepthStencilStateCI(true, true, VK_COMPARE_OP_LESS_OR_EQUAL),
		vk::makePipelineMultisampleStateCI(targets.msColor.samples));
	techniques.addTechnique("transparent"_id, device, allocator, targets.renderPass,
		descriptorPool, world.getDescriptorSets(),
		vk::makePipelineRasterizationStateCI(VK_POLYGON_MODE_FILL, true),
		vk::makePipelineColorBlendAttachmentState(vk::BlendingMode::Normal),
		vk::makePipelineDepthStencilStateCI(true, false, VK_COMPARE_OP_LESS_OR_EQUAL),
		vk::makePipelineMultisampleStateCI(targets.msColor.samples));
	createBloomPipelines();
	createBloomPipelineDS();
}

void Engine::cleanupPipelines() {
	destroyBloomPipelineDS(bloom);
	destroyBloomPipelines();
	techniques.destroy(device, allocator);
	destroyPresentPipelineDS(present);
	destroyPresentPipeline();
}

void Engine::createSwapchain(VkSwapchainKHR old) {
	// Find the best swapchain options
	auto const surfaceFormat = [&]() -> VkSurfaceFormatKHR {
		for (auto const& fmt: surfaceFormats) {
			if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB &&
				fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return fmt;
		}
		return surfaceFormats[0];
	}();
	auto const surfacePresentMode = [&]() -> VkPresentModeKHR {
		for (auto const mode: surfacePresentModes) {
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
				return mode;
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}();
	while (true) {
		if (window.isClosing())
			throw std::runtime_error{"Window close detected during a resize/minimized state"};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);
		auto const windowSize = window.size();
		swapchain.extent = VkExtent2D{
			.width = std::clamp(windowSize.x,
				surfaceCapabilities.minImageExtent.width,
				surfaceCapabilities.maxImageExtent.width),
			.height = std::clamp(windowSize.y,
				surfaceCapabilities.minImageExtent.height,
				surfaceCapabilities.maxImageExtent.height),
		};
		if (swapchain.extent.width != 0 && swapchain.extent.height != 0) break;
		std::this_thread::sleep_for(10_ms);
	}
	auto const maxImageCount = surfaceCapabilities.maxImageCount?: 256;
	auto const surfaceImageCount = std::min(surfaceCapabilities.minImageCount + 1, maxImageCount);

	// Create the swapchain
	auto const queueIndices = std::array{graphicsQueueFamilyIndex, presentQueueFamilyIndex};
	auto swapchainCI = VkSwapchainCreateInfoKHR{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = surface,
		.minImageCount = surfaceImageCount,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = swapchain.extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = uniquePresentQueue()? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = static_cast<u32>(uniquePresentQueue()? queueIndices.size() : 0_zu),
		.pQueueFamilyIndices = uniquePresentQueue()? queueIndices.data() : nullptr,
		.preTransform = surfaceCapabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = surfacePresentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = old,
	};
	VK(vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapchain.swapchain));

	// Retrieve swapchain images
	u32 swapchainImageCount;
	vkGetSwapchainImagesKHR(device, swapchain.swapchain, &swapchainImageCount, nullptr);
	auto swapchainImagesRaw = std::vector<VkImage>{swapchainImageCount};
	vkGetSwapchainImagesKHR(device, swapchain.swapchain, &swapchainImageCount, swapchainImagesRaw.data());
	swapchain.color.resize(swapchainImagesRaw.size());
	for (auto[raw, color]: zip_view{swapchainImagesRaw, swapchain.color}) {
		color = vk::Image{
			.image = raw,
			.format = surfaceFormat.format,
			.aspect = VK_IMAGE_ASPECT_COLOR_BIT,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.size = swapchain.extent,
		};
		color.view = vk::createImageView(device, color);
	}
}

void Engine::destroySwapchain(Swapchain& sc) {
	for (auto& image: sc.color)
		vk::destroyImage(device, allocator, image);
	vkDestroySwapchainKHR(device, sc.swapchain, nullptr);
}

void Engine::recreateSwapchain() {
	// Reobtain surface details
	u32 surfaceFormatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount,
		nullptr);
	surfaceFormats.resize(surfaceFormatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount,
		surfaceFormats.data());

	u32 surfacePresentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &surfacePresentModeCount,
		nullptr);
	surfacePresentModes.resize(surfaceFormatCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &surfacePresentModeCount,
		surfacePresentModes.data());

	ASSERT(!surfaceFormats.empty() && !surfacePresentModes.empty());

	// Queue up outdated objects for destruction
	delayedOps.emplace_back(DelayedOp{
		.deadline = frameCounter + FramesInFlight,
		.func = [this, swapchain=swapchain, targets=targets, present=present, bloom=bloom]() mutable {
			destroyPresentPipelineDS(present);
			destroyBloomPipelineDS(bloom);
			destroyBloomFbs(bloom);
			destroyTargetFbs(targets);
			destroyPresentFbs(present);
			destroyBloomImages(bloom);
			destroyTargetImages(targets);
			destroySwapchain(swapchain);
		},
	});

	// Create new objects
	auto oldSwapchain = swapchain.swapchain;
	swapchain = {};
	targets = {};
	createSwapchain(oldSwapchain);
	createTargetImages();
	createBloomImages();
	createPresentFbs();
	createTargetFbs();
	createBloomFbs();
	createBloomPipelineDS();
	createPresentPipelineDS();
}

void Engine::createPresentFbs() {
	// Create the present render pass
	present.renderPass = vk::createRenderPass(device, std::array{
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

	// Create the present framebuffers
	present.framebuffer.resize(swapchain.color.size());
	for (auto[fb, image]: zip_view{present.framebuffer, swapchain.color}) {
		fb = vk::createFramebuffer(device, present.renderPass, std::array{
			targets.ssColor,
			image,
		});
	}
}

void Engine::destroyPresentFbs(Present& p) {
	for (auto& fb: p.framebuffer)
		vkDestroyFramebuffer(device, fb, nullptr);
	vkDestroyRenderPass(device, p.renderPass, nullptr);
}

void Engine::createPresentPipeline() {
	present.descriptorSetLayout = vk::createDescriptorSetLayout(device, std::array{
		vk::Descriptor{
			.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
			.stages = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
	});
	present.shader = vk::createShader(device, presentVertSrc, presentFragSrc);

	present.layout = vk::createPipelineLayout(device, std::array{
		world.getDescriptorSetLayout(),
		present.descriptorSetLayout,
	});
	present.pipeline = vk::PipelineBuilder{
		.shader = present.shader,
		.vertexInputStateCI = vk::makePipelineVertexInputStateCI(),
		.inputAssemblyStateCI = vk::makePipelineInputAssemblyStateCI(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST),
		.rasterizationStateCI = vk::makePipelineRasterizationStateCI(VK_POLYGON_MODE_FILL, false),
		.colorBlendAttachmentState = vk::makePipelineColorBlendAttachmentState(vk::BlendingMode::None),
		.depthStencilStateCI = vk::makePipelineDepthStencilStateCI(false, false, VK_COMPARE_OP_ALWAYS),
		.multisampleStateCI = vk::makePipelineMultisampleStateCI(),
		.layout = present.layout,
	}.build(device, present.renderPass);
}

void Engine::destroyPresentPipeline() {
	vkDestroyPipeline(device, present.pipeline, nullptr);
	vkDestroyPipelineLayout(device, present.layout, nullptr);
	vk::destroyShader(device, present.shader);
	vkDestroyDescriptorSetLayout(device, present.descriptorSetLayout, nullptr);
}

void Engine::createPresentPipelineDS() {
	present.descriptorSet = vk::allocateDescriptorSet(device, descriptorPool, present.descriptorSetLayout);
	vk::updateDescriptorSets(device, std::array{
		vk::makeDescriptorSetImageWrite(present.descriptorSet, 0, targets.ssColor,
			VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
	});
}

void Engine::destroyPresentPipelineDS(Present& p) {
	vkFreeDescriptorSets(device, descriptorPool, 1, &p.descriptorSet);
}

void Engine::createMeshBuffer(VkCommandBuffer cmdBuf, std::vector<sys::vk::Buffer>& staging) {
	meshes.addMesh("block"_id, generateNormals(mesh::Block));
	meshes.addMesh("scene"_id, generateNormals(mesh::Scene));
	meshes.upload(allocator, cmdBuf, staging.emplace_back());
}

void Engine::destroyMeshBuffer() {
	meshes.destroy(allocator);
}

void Engine::createTargetImages() {
	auto const supportedSampleCount = deviceProperties.limits.framebufferColorSampleCounts & deviceProperties.limits.framebufferDepthSampleCounts;
	auto const selectedSampleCount = [=]() {
		if (SampleCount & supportedSampleCount) {
			return SampleCount;
		} else {
			L.warn("Requested antialiasing mode MSAA {}x not supported; defaulting to MSAA 2x",
				SampleCount);
			return VK_SAMPLE_COUNT_2_BIT;
		}
	}();
	targets.msColor = vk::createImage(device, allocator, ColorFormat, VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		swapchain.extent, selectedSampleCount);
	vk::setDebugName(device, targets.msColor.image, VK_OBJECT_TYPE_IMAGE, "targets.msColor");
	vk::setDebugName(device, targets.msColor.view, VK_OBJECT_TYPE_IMAGE_VIEW, "targets.msColor.view");
	targets.ssColor = vk::createImage(device, allocator, ColorFormat, VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
		swapchain.extent);
	vk::setDebugName(device, targets.ssColor.image, VK_OBJECT_TYPE_IMAGE, "targets.ssColor");
	vk::setDebugName(device, targets.ssColor.view, VK_OBJECT_TYPE_IMAGE_VIEW, "targets.ssColor.view");
	targets.depthStencil = vk::createImage(device, allocator, DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		swapchain.extent, selectedSampleCount);
	vk::setDebugName(device, targets.depthStencil.image, VK_OBJECT_TYPE_IMAGE, "targets.depthStencil");
	vk::setDebugName(device, targets.depthStencil.view, VK_OBJECT_TYPE_IMAGE_VIEW, "targets.depthStencil.view");
}

void Engine::destroyTargetImages(RenderTargets& t) {
	vk::destroyImage(device, allocator, t.depthStencil);
	vk::destroyImage(device, allocator, t.ssColor);
	vk::destroyImage(device, allocator, t.msColor);
}

void Engine::createTargetFbs() {
	targets.renderPass = vk::createRenderPass(device, std::array{
		vk::Attachment{
			.type = vk::Attachment::Type::Color,
			.image = targets.msColor,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.layoutDuring = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		},
		vk::Attachment{
			.type = vk::Attachment::Type::DepthStencil,
			.image = targets.depthStencil,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.layoutDuring = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		},
		vk::Attachment{
			.type = vk::Attachment::Type::Resolve,
			.image = targets.ssColor,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.layoutDuring = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		},
	});
	targets.framebuffer = vk::createFramebuffer(device, targets.renderPass, std::array{
		targets.msColor,
		targets.depthStencil,
		targets.ssColor,
	});
}

void Engine::destroyTargetFbs(RenderTargets& t) {
	vkDestroyFramebuffer(device, t.framebuffer, nullptr);
	vkDestroyRenderPass(device, t.renderPass, nullptr);
}

void Engine::createBloomImages() {
	auto extent = swapchain.extent;
	for (auto& image: bloom.images) {
		image = vk::createImage(device, allocator, ColorFormat, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, extent);
		vk::setDebugName(device, image.image, VK_OBJECT_TYPE_IMAGE, fmt::format("bloom.images[{}]", &image - &bloom.images[0]));
		vk::setDebugName(device, image.view, VK_OBJECT_TYPE_IMAGE_VIEW, fmt::format("bloom.images[{}].view", &image - &bloom.images[0]));
		extent.width = std::max(1u, extent.width >> 1);
		extent.height = std::max(1u, extent.height >> 1);
	}
}

void Engine::destroyBloomImages(Bloom& b) {
	for (auto& image: b.images)
		vk::destroyImage(device, allocator, image);
}

void Engine::createBloomFbs() {
	bloom.downPass = vk::createRenderPass(device, std::array{
		vk::Attachment{
			.type = vk::Attachment::Type::Color,
			.image = bloom.images[0],
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.layoutDuring = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		},
	});
	bloom.upPass = vk::createRenderPass(device, std::array{
		vk::Attachment{
			.type = vk::Attachment::Type::Color,
			.image = bloom.images[0],
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.layoutBefore = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		},
	});

	bloom.targetFb = vk::createFramebuffer(device, bloom.downPass, std::array{targets.ssColor});
	for (auto[fb, image]: zip_view{bloom.imageFbs, bloom.images})
		fb = vk::createFramebuffer(device, bloom.downPass, std::array{image});
}

void Engine::destroyBloomFbs(Bloom& b) {
	vkDestroyFramebuffer(device, b.targetFb, nullptr);
	for (auto fb: b.imageFbs)
		vkDestroyFramebuffer(device, fb, nullptr);
	vkDestroyRenderPass(device, b.upPass, nullptr);
	vkDestroyRenderPass(device, b.downPass, nullptr);
}

void Engine::createBloomPipelines() {
	bloom.descriptorSetLayout = vk::createDescriptorSetLayout(device, std::array{
		vk::Descriptor{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.stages = VK_SHADER_STAGE_FRAGMENT_BIT,
			.sampler = linear,
		},
	});
	bloom.shader = vk::createShader(device, bloomVertSrc, bloomFragSrc);

	bloom.layout = vk::createPipelineLayout(device, std::array{
		world.getDescriptorSetLayout(),
		bloom.descriptorSetLayout,
	});
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
	bloom.down = builder.build(device, bloom.downPass);
	builder.colorBlendAttachmentState = vk::makePipelineColorBlendAttachmentState(vk::BlendingMode::Add);
	bloom.up = builder.build(device, bloom.upPass);
}

void Engine::destroyBloomPipelines() {
	vkDestroyPipeline(device, bloom.up, nullptr);
	vkDestroyPipeline(device, bloom.down, nullptr);
	vkDestroyPipelineLayout(device, bloom.layout, nullptr);
	vk::destroyShader(device, bloom.shader);
	vkDestroyDescriptorSetLayout(device, bloom.descriptorSetLayout, nullptr);
}

void Engine::createBloomPipelineDS() {
	bloom.sourceDS = vk::allocateDescriptorSet(device, descriptorPool, bloom.descriptorSetLayout);
	for (auto& ds: bloom.imageDS)
		ds = vk::allocateDescriptorSet(device, descriptorPool, bloom.descriptorSetLayout);

	vk::updateDescriptorSets(device, std::array{
		vk::makeDescriptorSetImageWrite(bloom.sourceDS, 0, targets.ssColor,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
	});
	for (auto[ds, image]: zip_view{bloom.imageDS, bloom.images}) {
		vk::updateDescriptorSets(device, std::array{
			vk::makeDescriptorSetImageWrite(ds, 0, image,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
		});
	}
}

void Engine::destroyBloomPipelineDS(Bloom& b) {
	for (auto& ds: b.imageDS)
		vkFreeDescriptorSets(device, descriptorPool, 1, &ds);
	vkFreeDescriptorSets(device, descriptorPool, 1, &b.sourceDS);
}

#ifdef VK_VALIDATION
VKAPI_ATTR auto VKAPI_CALL Engine::debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severityCode,
	VkDebugUtilsMessageTypeFlagsEXT typeCode,
	VkDebugUtilsMessengerCallbackDataEXT const* data,
	void*) -> VkBool32 {
	ASSERT(data);

	auto const severity = [=] {
		if (severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
			return Log::Level::Error;
		if (severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
			return Log::Level::Warn;
		if (severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
			return Log::Level::Info;
		if (severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
			return Log::Level::Debug;
		throw std::logic_error{fmt::format("Unknown Vulkan diagnostic message severity: #{}",
			severityCode)};
	}();

	auto type = [=]() -> std::string_view {
		if (typeCode & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
			return "[VulkanPerf]";
		if (typeCode & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
			return "[VulkanSpec]";
		if (typeCode & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
			return "[Vulkan]";
		throw std::logic_error{fmt::format("Unknown Vulkan diagnostic message type: #{}",
			typeCode)};
	}();

	L.log(severity, "{} {}", type, data->pMessage);

	return VK_FALSE;
}

#endif //VK_VALIDATION

}

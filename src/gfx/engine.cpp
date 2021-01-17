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
#include "sys/vk/pipeline.hpp"
#include "sys/vk/base.hpp"
#include "sys/window.hpp"
#include "sys/glfw.hpp"
#include "gfx/blur.hpp"
#include "mesh/block.hpp"
#include "mesh/scene.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;
namespace vk = sys::vk;
namespace ranges = std::ranges;
using namespace std::string_literals;

constexpr auto passthroughVertSrc = std::to_array<u32>({
#include "spv/passthrough.vert.spv"
});

constexpr auto passthroughFragSrc = std::to_array<u32>({
#include "spv/passthrough.frag.spv"
});

constexpr auto bloomVertSrc = std::to_array<u32>({
#include "spv/bloom.vert.spv"
});

constexpr auto bloomFragSrc = std::to_array<u32>({
#include "spv/bloom.frag.spv"
});

static constexpr auto versionToCode(Version version) -> u32 {
	return (get<0>(version) << 22) + (get<1>(version) << 12) + get<2>(version);
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
	world.ambientColor = glm::vec4{color, 1.0f};
}

void Engine::setLightSource(glm::vec3 position, glm::vec3 color) {
	world.lightPosition = glm::vec4{position, 1.0f};
	world.lightColor = glm::vec4{color, 1.0f};
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

	world.setViewProjection(glm::uvec2{swapchain.extent.width, swapchain.extent.height},
		VerticalFov, NearPlane, FarPlane,
		camera.eye, camera.center, camera.up);
	uploadToCpuBuffer(allocator, techniques.getWorldConstants(frameIndex), world);

	auto const viewport = VkViewport{
		.width = static_cast<f32>(swapchain.extent.width),
		.height = static_cast<f32>(swapchain.extent.height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	auto const scissor = VkRect2D{
		.extent = swapchain.extent,
	};

	// Start recording commands
	VK(vkResetCommandBuffer(frame.commandBuffer, 0));
	auto cmdBeginInfo = VkCommandBufferBeginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	VK(vkBeginCommandBuffer(frame.commandBuffer, &cmdBeginInfo));

	// Compute clear color
	auto const clearValues = std::to_array<VkClearValue>({
		{ .color = { .float32 = {world.ambientColor.r, world.ambientColor.g, world.ambientColor.b, world.ambientColor.a} }},
		{ .depthStencil = { .depth = 1.0f }},
	});

	// Start the object drawing pass
	VkRenderPassBeginInfo rpBeginInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = targets.renderPass,
		.framebuffer = targets.framebuffer,
		.renderArea = {
			.extent = swapchain.extent,
		},
		.clearValueCount = clearValues.size(),
		.pClearValues = clearValues.data(),
	};
	vkCmdBeginRenderPass(frame.commandBuffer, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdSetViewport(frame.commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(frame.commandBuffer, 0, 1, &scissor);

	// Opaque object draw
	vkCmdBindPipeline(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, opaque.pipeline);
	vkCmdBindDescriptorSets(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		techniques.getPipelineLayout(), 0,
		opaque.descriptorSets[frameIndex].size(), opaque.descriptorSets[frameIndex].data(),
		0, nullptr);

	vkCmdDrawIndirect(frame.commandBuffer, opaqueIndirect.commandBuffer(), 0, opaqueIndirect.size(), sizeof(IndirectBuffer::Command));

	// Transparent object draw prepass
	vkCmdBindPipeline(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, transparentDepthPrepass.pipeline);
	vkCmdBindDescriptorSets(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		techniques.getPipelineLayout(), 0,
		transparentDepthPrepass.descriptorSets[frameIndex].size(), transparentDepthPrepass.descriptorSets[frameIndex].data(),
		0, nullptr);

	vkCmdDrawIndirect(frame.commandBuffer, transparentDepthPrepassIndirect.commandBuffer(), 0, transparentDepthPrepassIndirect.size(), sizeof(IndirectBuffer::Command));

	// Transparent object draw
	vkCmdBindPipeline(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, transparent.pipeline);
	vkCmdBindDescriptorSets(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		techniques.getPipelineLayout(), 0,
		transparent.descriptorSets[frameIndex].size(), transparent.descriptorSets[frameIndex].data(),
		0, nullptr);

	vkCmdDrawIndirect(frame.commandBuffer, transparentIndirect.commandBuffer(), 0, transparentIndirect.size(), sizeof(IndirectBuffer::Command));

	// Finish the object drawing pass
	vkCmdEndRenderPass(frame.commandBuffer);

	// Synchronize the rendered color image
	auto imageMemoryBarrier = VkImageMemoryBarrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.image = targets.ssColor.image,
		.subresourceRange = VkImageSubresourceRange{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};
	vkCmdPipelineBarrier(frame.commandBuffer,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

	// Blit the image to screen
	rpBeginInfo = VkRenderPassBeginInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = present.renderPass,
		.framebuffer = present.framebuffer[swapchainImageIndex],
		.renderArea = {
			.extent = swapchain.extent,
		},
	};
	vkCmdBeginRenderPass(frame.commandBuffer, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, present.pipeline);
	vkCmdBindDescriptorSets(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		present.layout, 0, 1, &present.descriptorSet, 0, nullptr);
	vkCmdDraw(frame.commandBuffer, 3, 1, 0, 0);
	vkCmdEndRenderPass(frame.commandBuffer);

	// Finish recording commands
	VK(vkEndCommandBuffer(frame.commandBuffer));

	// Submit commands to the queue
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	auto submitInfo = VkSubmitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &frame.presentSemaphore,
		.pWaitDstStageMask = &waitStage,
		.commandBufferCount = 1,
		.pCommandBuffers = &frame.commandBuffer,
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
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 16 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 16 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 16 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16 },
	});
	auto descriptorPoolCI = VkDescriptorPoolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = 32,
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
}

void Engine::cleanupImages() {
	destroyTargetImages(targets);
}

void Engine::initFramebuffers() {
	createPresentFbs();
	createTargetFbs();
}

void Engine::cleanupFramebuffers() {
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
}

void Engine::cleanupBuffers() {
	destroyMeshBuffer();
}

void Engine::initPipelines() {
	createPresentPipeline();
	createPresentPipelineDS();

	techniques.create(device, allocator, descriptorPool, meshes);
	techniques.addTechnique("opaque"_id, device, allocator, descriptorPool, targets.renderPass,
		vk::makePipelineRasterizationStateCI(VK_POLYGON_MODE_FILL, true),
		vk::makePipelineColorBlendAttachmentState(vk::BlendingMode::None),
		vk::makePipelineDepthStencilStateCI(true, true, VK_COMPARE_OP_LESS_OR_EQUAL),
		vk::makePipelineMultisampleStateCI(targets.sampleCount));
	techniques.addTechnique("transparent_depth_prepass"_id, device, allocator, descriptorPool, targets.renderPass,
		vk::makePipelineRasterizationStateCI(VK_POLYGON_MODE_FILL, true),
		vk::makePipelineColorBlendAttachmentState(vk::BlendingMode::None, false),
		vk::makePipelineDepthStencilStateCI(true, true, VK_COMPARE_OP_LESS_OR_EQUAL),
		vk::makePipelineMultisampleStateCI(targets.sampleCount));
	techniques.addTechnique("transparent"_id, device, allocator, descriptorPool, targets.renderPass,
		vk::makePipelineRasterizationStateCI(VK_POLYGON_MODE_FILL, true),
		vk::makePipelineColorBlendAttachmentState(vk::BlendingMode::Normal),
		vk::makePipelineDepthStencilStateCI(true, false, VK_COMPARE_OP_LESS_OR_EQUAL),
		vk::makePipelineMultisampleStateCI(targets.sampleCount));
}

void Engine::cleanupPipelines() {
	techniques.destroy(device, allocator);

	destroyPresentPipelineDS(present);
	destroyPresentPipeline(present);
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
	swapchain.format = surfaceFormat.format;
	u32 swapchainImageCount;
	vkGetSwapchainImagesKHR(device, swapchain.swapchain, &swapchainImageCount, nullptr);
	auto swapchainImagesRaw = std::vector<VkImage>{swapchainImageCount};
	vkGetSwapchainImagesKHR(device, swapchain.swapchain, &swapchainImageCount, swapchainImagesRaw.data());
	swapchain.color.resize(swapchainImagesRaw.size());
	swapchain.colorView.resize(swapchainImagesRaw.size());
	for (auto[raw, color, cv]: zip_view{swapchainImagesRaw, swapchain.color, swapchain.colorView}) {
		color = vk::Image{
			.image = raw,
			.format = swapchain.format,
		};
		cv = vk::createImageView(device, color, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

void Engine::destroySwapchain(Swapchain& sc) {
	for (auto& cv: sc.colorView)
		vkDestroyImageView(device, cv, nullptr);
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
		.func = [this, swapchain=swapchain, targets=targets, present=present]() mutable {
			destroyPresentPipelineDS(present);
			destroyTargetFbs(targets);
			destroyPresentFbs(present);
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
	createPresentFbs();
	createTargetFbs();
	createPresentPipelineDS();
}

void Engine::createPresentFbs() {
	// Create the present render pass
	auto const rpAttachments = std::to_array<VkAttachmentDescription>({
		{ // Source
			.format = targets.ssColor.format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		},
		{ // Present
			.format = swapchain.format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		},
	});
	auto const sourceAttachmentRef = VkAttachmentReference{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};
	auto const presentAttachmentRef = VkAttachmentReference{
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};
	auto const subpass = VkSubpassDescription{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 1,
		.pInputAttachments = &sourceAttachmentRef,
		.colorAttachmentCount = 1,
		.pColorAttachments = &presentAttachmentRef,
	};
	auto const renderPassCI = VkRenderPassCreateInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = rpAttachments.size(),
		.pAttachments = rpAttachments.data(),
		.subpassCount = 1,
		.pSubpasses = &subpass,
	};
	VK(vkCreateRenderPass(device, &renderPassCI, nullptr, &present.renderPass));

	// Create the present framebuffers
	present.framebuffer.resize(swapchain.color.size());
	for (auto[fb, cv]: zip_view{present.framebuffer, swapchain.colorView}) {
		auto const fbAttachments = std::to_array<VkImageView>({
			targets.ssColorView,
			cv,
		});
		auto const framebufferCI = VkFramebufferCreateInfo{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = present.renderPass,
			.attachmentCount = fbAttachments.size(),
			.pAttachments = fbAttachments.data(),
			.width = swapchain.extent.width,
			.height = swapchain.extent.height,
			.layers = 1,
		};
		VK(vkCreateFramebuffer(device, &framebufferCI, nullptr, &fb));
	}
}

void Engine::destroyPresentFbs(Present& p) {
	for (auto& fb: p.framebuffer)
		vkDestroyFramebuffer(device, fb, nullptr);
	vkDestroyRenderPass(device, p.renderPass, nullptr);
}

void Engine::createPresentPipeline() {
	auto const presentImageBinding = VkDescriptorSetLayoutBinding{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
	};
	auto const descriptorSetLayoutCI = VkDescriptorSetLayoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &presentImageBinding,
	};
	present.shader = vk::createShader(device, passthroughVertSrc, passthroughFragSrc,
		std::array{descriptorSetLayoutCI});

	present.layout = vk::createPipelineLayout(device, std::to_array({
		present.shader.descriptorSetLayouts[0],
	}));
	present.pipeline = vk::PipelineBuilder{
		.shaderStageCIs = {
			vk::makePipelineShaderStageCI(VK_SHADER_STAGE_VERTEX_BIT, present.shader.vert),
			vk::makePipelineShaderStageCI(VK_SHADER_STAGE_FRAGMENT_BIT, present.shader.frag),
		},
		.vertexInputStateCI = vk::makePipelineVertexInputStateCI(),
		.inputAssemblyStateCI = vk::makePipelineInputAssemblyStateCI(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST),
		.rasterizationStateCI = vk::makePipelineRasterizationStateCI(VK_POLYGON_MODE_FILL, false),
		.colorBlendAttachmentState = vk::makePipelineColorBlendAttachmentState(vk::BlendingMode::None),
		.depthStencilStateCI = vk::makePipelineDepthStencilStateCI(false, false, VK_COMPARE_OP_ALWAYS),
		.multisampleStateCI = vk::makePipelineMultisampleStateCI(),
		.layout = present.layout,
	}.build(device, present.renderPass);
}

void Engine::destroyPresentPipeline(Present& p) {
	vkDestroyPipeline(device, p.pipeline, nullptr);
	vkDestroyPipelineLayout(device, p.layout, nullptr);
	vk::destroyShader(device, p.shader);
}

void Engine::createPresentPipelineDS() {
	auto const descriptorSetAI = VkDescriptorSetAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &present.shader.descriptorSetLayouts[0],
	};
	VK(vkAllocateDescriptorSets(device, &descriptorSetAI, &present.descriptorSet));

	auto const descriptorImageInfo = VkDescriptorImageInfo{
		.imageView = targets.ssColorView,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};
	auto const writeDS = VkWriteDescriptorSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = present.descriptorSet,
		.dstBinding = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
		.pImageInfo = &descriptorImageInfo,
	};
	vkUpdateDescriptorSets(device, 1, &writeDS, 0, nullptr);
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
	if (SampleCount & supportedSampleCount) {
		targets.sampleCount = SampleCount;
	} else {
		L.warn("Requested antialiasing mode MSAA {}x not supported; defaulting to MSAA 2x", SampleCount);
		targets.sampleCount = VK_SAMPLE_COUNT_2_BIT;
	}
	targets.msColor = vk::createImage(allocator, ColorFormat,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		swapchain.extent, targets.sampleCount);
	vk::setDebugName(device, targets.msColor.image, VK_OBJECT_TYPE_IMAGE, "targets.msColor");
	targets.msColorView = vk::createImageView(device, targets.msColor,
		VK_IMAGE_ASPECT_COLOR_BIT);
	vk::setDebugName(device, targets.msColorView, VK_OBJECT_TYPE_IMAGE_VIEW, "targets.msColorView");
	targets.ssColor = vk::createImage(allocator, ColorFormat,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
		swapchain.extent, VK_SAMPLE_COUNT_1_BIT);
	vk::setDebugName(device, targets.ssColor.image, VK_OBJECT_TYPE_IMAGE, "targets.ssColor");
	targets.ssColorView = vk::createImageView(device, targets.ssColor,
		VK_IMAGE_ASPECT_COLOR_BIT);
	vk::setDebugName(device, targets.ssColorView, VK_OBJECT_TYPE_IMAGE_VIEW, "targets.ssColorView");
	targets.depthStencil = vk::createImage(allocator, DepthFormat,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		swapchain.extent, targets.sampleCount);
	vk::setDebugName(device, targets.depthStencil.image, VK_OBJECT_TYPE_IMAGE, "targets.depthStencil");
	targets.depthStencilView = vk::createImageView(device, targets.depthStencil,
		VK_IMAGE_ASPECT_DEPTH_BIT);
	vk::setDebugName(device, targets.depthStencilView, VK_OBJECT_TYPE_IMAGE_VIEW, "targets.depthStencilView");
}

void Engine::destroyTargetImages(RenderTargets& t) {
	vkDestroyImageView(device, t.depthStencilView, nullptr);
	vmaDestroyImage(allocator, t.depthStencil.image, t.depthStencil.allocation);
	vkDestroyImageView(device, t.ssColorView, nullptr);
	vmaDestroyImage(allocator, t.ssColor.image, t.ssColor.allocation);
	vkDestroyImageView(device, t.msColorView, nullptr);
	vmaDestroyImage(allocator, t.msColor.image, t.msColor.allocation);
}

void Engine::createTargetFbs() {
	auto const rpAttachments = std::to_array<VkAttachmentDescription>({
		{ // MSAA color
			.format = targets.msColor.format,
			.samples = targets.sampleCount,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		},
		{ // MSAA depth
			.format = targets.depthStencil.format,
			.samples = targets.sampleCount,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		},
		{ // Resolve
			.format = targets.ssColor.format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		},
	});
	auto const colorAttachmentRef = VkAttachmentReference{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};
	auto const depthAttachmentRef = VkAttachmentReference{
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};
	auto const resolveAttachmentRef = VkAttachmentReference{
		.attachment = 2,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};
	auto const subpass = VkSubpassDescription{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		.pResolveAttachments = &resolveAttachmentRef,
		.pDepthStencilAttachment = &depthAttachmentRef,
	};
	auto const renderPassCI = VkRenderPassCreateInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = rpAttachments.size(),
		.pAttachments = rpAttachments.data(),
		.subpassCount = 1,
		.pSubpasses = &subpass,
	};
	VK(vkCreateRenderPass(device, &renderPassCI, nullptr, &targets.renderPass));

	auto const fbAttachments = std::to_array<VkImageView>({
		targets.msColorView,
		targets.depthStencilView,
		targets.ssColorView,
	});
	auto const framebufferCI = VkFramebufferCreateInfo{
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass = targets.renderPass,
		.attachmentCount = fbAttachments.size(),
		.pAttachments = fbAttachments.data(),
		.width = swapchain.extent.width,
		.height = swapchain.extent.height,
		.layers = 1,
	};
	VK(vkCreateFramebuffer(device, &framebufferCI, nullptr, &targets.framebuffer));
}

void Engine::destroyTargetFbs(RenderTargets& t) {
	vkDestroyFramebuffer(device, t.framebuffer, nullptr);
	vkDestroyRenderPass(device, t.renderPass, nullptr);
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

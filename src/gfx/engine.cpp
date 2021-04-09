#include "gfx/engine.hpp"

#include "config.hpp"

#include <stdexcept>
#include <cstring>
#include <cassert>
#include "VkBootstrap.h"
#include "GLFW/glfw3.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "fmt/core.h"
#include "volk.h"
#include "vuk/CommandBuffer.hpp"
#include "vuk/RenderGraph.hpp"
#include "base/math.hpp"
#include "base/log.hpp"
#include "gfx/base.hpp"
#include "main.hpp"

namespace minote::gfx {

using namespace base;
using namespace std::string_literals;

#if VK_VALIDATION
VKAPI_ATTR auto VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severityCode,
	VkDebugUtilsMessageTypeFlagsEXT typeCode,
	VkDebugUtilsMessengerCallbackDataEXT const* data,
	void*) -> VkBool32 {
	assert(data);

	auto const severity = [severityCode] {
		if (severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
			return Log::Level::Error;
		if (severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
			return Log::Level::Warn;
		if (severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
			return Log::Level::Info;
		if (severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
			return Log::Level::Debug;
		throw std::logic_error(fmt::format("Unknown Vulkan diagnostic message severity: #{}", severityCode));
	}();

	auto type = [typeCode]() {
		if (typeCode & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
			return "[VulkanPerf]";
		if (typeCode & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
			return "[VulkanSpec]";
		if (typeCode & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
			return "[Vulkan]";
		throw std::logic_error(fmt::format("Unknown Vulkan diagnostic message type: #{}", typeCode));
	}();

	L.log(severity, "{} {}", type, data->pMessage);

	return VK_FALSE;
}
#endif //VK_VALIDATION

Engine::Engine(sys::Window& window, Version version) {
	// Create instance
	auto instanceResult = vkb::InstanceBuilder()
#if VK_VALIDATION
		.request_validation_layers()
		.set_debug_callback(debugCallback)
#endif //VK_VALIDATION
		.set_app_name(AppTitle)
		.set_engine_name("vuk")
		.require_api_version(1, 2, 0)
		.set_app_version(std::get<0>(version), std::get<1>(version), std::get<2>(version))
		.build();
	if (!instanceResult)
		throw std::runtime_error(fmt::format("Failed to create a Vulkan instance: {}", instanceResult.error().message()));
	instance = instanceResult.value();
	volkInitializeCustom(instance.fp_vkGetInstanceProcAddr);
	volkLoadInstanceOnly(instance.instance);
	L.debug("Vulkan instance created");

	// Create surface
	glfwCreateWindowSurface(instance.instance, window.handle(), nullptr, &surface);

	// Select physical device
	auto physicalDeviceFeatures = VkPhysicalDeviceFeatures{
		.multiDrawIndirect = true,
	};
	auto physicalDeviceSelectorResult = vkb::PhysicalDeviceSelector(instance)
		.set_surface(surface)
		.set_minimum_version(1, 0)
		.set_required_features(physicalDeviceFeatures)
		.select();
	if (!physicalDeviceSelectorResult)
		throw std::runtime_error(fmt::format("Failed to find a suitable GPU for Vulkan: {}", physicalDeviceSelectorResult.error().message()));
	auto physicalDevice = physicalDeviceSelectorResult.value();

	// Create device
	auto queryReset = VkPhysicalDeviceHostQueryResetFeatures{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES,
		.hostQueryReset = VK_TRUE,
	};
	auto deviceResult = vkb::DeviceBuilder(physicalDevice)
		.add_pNext(&queryReset)
		.build();
	if (!deviceResult)
		throw std::runtime_error(fmt::format("Failed to create Vulkan device: {}", deviceResult.error().message()));
	device = deviceResult.value();
	volkLoadDevice(device.device);
	L.debug("Vulkan device created");

	// Get queues
	auto graphicsQueue = device.get_queue(vkb::QueueType::graphics).value();
	auto graphicsQueueFamilyIndex = device.get_queue_index(vkb::QueueType::graphics).value();
	auto transferQueuePresent = device.get_dedicated_queue(vkb::QueueType::present).has_value();
	auto transferQueue = transferQueuePresent? device.get_dedicated_queue(vkb::QueueType::present).value() : VK_NULL_HANDLE;
	auto transferQueueFamilyIndex = transferQueuePresent? device.get_dedicated_queue_index(vkb::QueueType::present).value() : VK_QUEUE_FAMILY_IGNORED;

	// Create vuk context
	context.emplace(vuk::ContextCreateParameters{
		instance.instance, device.device, physicalDevice.physical_device,
		graphicsQueue, graphicsQueueFamilyIndex,
		transferQueue, transferQueueFamilyIndex
	});

	// Create swapchain
	swapchain = context->add_swapchain(createSwapchain());
}

Engine::~Engine() {
	context->wait_idle();
	env.reset();
	indicesBuf.reset();
	colorsBuf.reset();
	normalsBuf.reset();
	verticesBuf.reset();
#if IMGUI
	imguiData.font_texture.view.reset();
	imguiData.font_texture.image.reset();
#endif //IMGUI
	// This performs cleanups for all inflight frames
	for (auto i = 0u; i < vuk::Context::FC; i++)
		context->begin();
	context.reset();
	vkb::destroy_device(device);
	vkDestroySurfaceKHR(instance.instance, surface, nullptr);
	vkb::destroy_instance(instance);
}

void Engine::addModel(std::string_view name, std::span<char const> model) {
	meshBuffer.addGltf(name, model);
}

void Engine::setup() {
	auto ifc = context->begin();
	auto ptc = ifc.begin();

	// Create pipelines

	auto objectPci = vuk::PipelineBaseCreateInfo();
	objectPci.add_spirv(std::vector<u32>{
#include "spv/object.vert.spv"
	}, "object.vert");
	objectPci.add_spirv(std::vector<u32>{
#include "spv/object.frag.spv"
	}, "object.frag");
	objectPci.rasterization_state.cullMode = vuk::CullModeFlagBits::eBack;
	context->create_named_pipeline("object", objectPci);

	auto cubemapPci = vuk::PipelineBaseCreateInfo();
	cubemapPci.add_spirv(std::vector<u32>{
#include "spv/cubemap.vert.spv"
	}, "cubemap.vert");
	cubemapPci.add_spirv(std::vector<u32>{
#include "spv/cubemap.frag.spv"
	}, "cubemap.frag");
	cubemapPci.depth_stencil_state.depthWriteEnable = false;
	context->create_named_pipeline("cubemap", cubemapPci);

	auto swapchainBlitPci = vuk::PipelineBaseCreateInfo();
	swapchainBlitPci.add_spirv(std::vector<u32>{
#include "spv/swapchainBlit.vert.spv"
	}, "blit.vert");
	swapchainBlitPci.add_spirv(std::vector<u32>{
#include "spv/swapchainBlit.frag.spv"
	}, "blit.frag");
	context->create_named_pipeline("swapchain_blit", swapchainBlitPci);

	auto bloomThresholdPci = vuk::PipelineBaseCreateInfo();
	bloomThresholdPci.add_spirv(std::vector<u32>{
#include "spv/bloomThreshold.vert.spv"
	}, "bloomThreshold.vert");
	bloomThresholdPci.add_spirv(std::vector<u32>{
#include "spv/bloomThreshold.frag.spv"
	}, "bloomThreshold.frag");
	context->create_named_pipeline("bloom_threshold", bloomThresholdPci);

	auto bloomBlurDownPci = vuk::PipelineBaseCreateInfo();
	bloomBlurDownPci.add_spirv(std::vector<u32>{
#include "spv/bloomBlur.vert.spv"
	}, "bloomBlur.vert");
	bloomBlurDownPci.add_spirv(std::vector<u32>{
#include "spv/bloomBlur.frag.spv"
	}, "bloomBlur.frag");
	context->create_named_pipeline("bloom_blur_down", bloomBlurDownPci);

	auto bloomBlurUpPci = vuk::PipelineBaseCreateInfo();
	bloomBlurUpPci.add_spirv(std::vector<u32>{
#include "spv/bloomBlur.vert.spv"
	}, "bloomBlur.vert");
	bloomBlurUpPci.add_spirv(std::vector<u32>{
#include "spv/bloomBlur.frag.spv"
	}, "bloomBlur.frag");
	bloomBlurUpPci.set_blend(vuk::BlendPreset::eAlphaBlend);
	// Turn into additive
	bloomBlurUpPci.color_blend_attachments[0].srcColorBlendFactor = vuk::BlendFactor::eOne;
	bloomBlurUpPci.color_blend_attachments[0].dstColorBlendFactor = vuk::BlendFactor::eOne;
	bloomBlurUpPci.color_blend_attachments[0].dstAlphaBlendFactor = vuk::BlendFactor::eOne;
	context->create_named_pipeline("bloom_blur_up", bloomBlurUpPci);

	// Initialize imgui rendering
#if IMGUI
	imguiData = ImGui_ImplVuk_Init(ptc);
	ImGui::GetIO().DisplaySize = ImVec2(f32(swapchain->extent.width), f32(swapchain->extent.height));
#endif //IMGUI

	// Upload mesh buffers
	auto [vertices, normals, colors, indices] = meshBuffer.makeBuffers();
	verticesBuf = ptc.create_buffer(vuk::MemoryUsage::eGPUonly,
		vuk::BufferUsageFlagBits::eVertexBuffer, std::span(vertices)).first;
	normalsBuf = ptc.create_buffer(vuk::MemoryUsage::eGPUonly,
		vuk::BufferUsageFlagBits::eVertexBuffer, std::span(normals)).first;
	colorsBuf = ptc.create_buffer(vuk::MemoryUsage::eGPUonly,
		vuk::BufferUsageFlagBits::eVertexBuffer, std::span(colors)).first;
	indicesBuf = ptc.create_buffer(vuk::MemoryUsage::eGPUonly,
		vuk::BufferUsageFlagBits::eIndexBuffer, std::span(indices)).first;

	// Upload environment map
	auto width = 0;
	auto height = 0;
	auto components = 0;
	stbi_set_flip_vertically_on_load(true);
	auto* hdri = stbi_loadf("env.hdr", &width, &height, &components, 4);
	env = context->allocate_texture(vuk::ImageCreateInfo{
		.format = vuk::Format::eR32G32B32A32Sfloat,
		.extent = {u32(width), u32(height), 1u},
		.mipLevels = u32(std::log2(std::max(width, height))) + 1,
		.usage = vuk::ImageUsageFlagBits::eTransferSrc | vuk::ImageUsageFlagBits::eTransferDst | vuk::ImageUsageFlagBits::eSampled,
	});
	ptc.upload(*env->image, vuk::Format::eR32G32B32A32Sfloat, {u32(width), u32(height), 1u}, 0,
		std::span(hdri, width * height * 4), true);
	stbi_image_free(hdri);

	// Finalize uploads
	ptc.wait_all_transfers();

	// Begin imgui frame so that first-frame calls succeed
#if IMGUI
	ImGui::NewFrame();
#endif //IMGUI
}

void Engine::setCamera(glm::vec3 eye, glm::vec3 center, glm::vec3 up) {
	camera = Camera{
		.eye = eye,
		.center = center,
		.up = up,
	};
}

void Engine::enqueue(ID mesh, std::span<Instance const> instances) {
	instanceBuffer.addInstances(mesh, instances);
}

void Engine::render() {
	// Prepare per-frame data
	auto viewport = glm::uvec2(swapchain->extent.width, swapchain->extent.height);
	auto rawview = glm::lookAt(camera.eye, camera.center, camera.up);
	auto yFlip = make_scale({-1.0f, -1.0f, 1.0f});
	world.projection = glm::infinitePerspective(VerticalFov, f32(viewport.x) / f32(viewport.y), NearPlane);
	world.view = yFlip * rawview;
	world.viewProjection = world.projection * world.view;

	constexpr auto BloomDepth = 6u;
	auto bloomNames = std::array<std::string, BloomDepth>{};
	for (auto i = 0u; i < BloomDepth; i += 1)
		bloomNames[i] = "bloom"s + std::to_string(i);
	auto swapchainSize = vuk::Dimension2D::absolute(swapchain->extent);
	auto bloomSizes = std::array<vuk::Dimension2D, BloomDepth>{};
	for (auto i = 0u; i < BloomDepth; i += 1)
		bloomSizes[i] = vuk::Dimension2D::absolute(std::max(swapchainSize.extent.width >> i, 1u), std::max(swapchainSize.extent.height >> i, 1u));

	// Begin draw
	auto ifc = context->begin();
	auto ptc = ifc.begin();

	// Upload uniform buffers
	auto worldBuf = ptc.allocate_scratch_buffer(
		vuk::MemoryUsage::eCPUtoGPU,
		vuk::BufferUsageFlagBits::eUniformBuffer,
		sizeof(World), alignof(World));
	std::memcpy(worldBuf.mapped_ptr, &world, sizeof(world));

	// Upload indirect buffers
	auto [commands, instances] = instanceBuffer.makeIndirect(meshBuffer);
	auto commandsBuf = ptc.allocate_scratch_buffer(
		vuk::MemoryUsage::eCPUtoGPU,
		vuk::BufferUsageFlagBits::eIndirectBuffer,
		sizeof(decltype(commands)::value_type) * commands.size(), alignof(decltype(commands)::value_type));
	std::memcpy(commandsBuf.mapped_ptr, commands.data(), sizeof(decltype(commands)::value_type) * commands.size());
	auto instancesBuf = ptc.allocate_scratch_buffer(
		vuk::MemoryUsage::eCPUtoGPU,
		vuk::BufferUsageFlagBits::eStorageBuffer,
		sizeof(decltype(instances)::value_type) * instances.size(), alignof(decltype(instances)::value_type));
	std::memcpy(instancesBuf.mapped_ptr, instances.data(), sizeof(decltype(instances)::value_type) * instances.size());

	// Set up the rendergraph
	auto rg = vuk::RenderGraph();
	rg.add_pass({ // Object draw
		.auxiliary_order = 0.0f,
		.resources = {"object_color"_image(vuk::eColorWrite), "object_depth"_image(vuk::eDepthStencilRW)},
		.execute = [this, worldBuf, &instancesBuf, &commands, &commandsBuf](vuk::CommandBuffer& command_buffer) {
			auto envSampler = vuk::SamplerCreateInfo{
				.magFilter = vuk::Filter::eLinear,
				.minFilter = vuk::Filter::eLinear,
				.mipmapMode = vuk::SamplerMipmapMode::eLinear,
			};
			command_buffer
				.set_viewport(0, vuk::Rect2D::framebuffer())
				.set_scissor(0, vuk::Rect2D::framebuffer())
				.bind_uniform_buffer(0, 0, worldBuf)
				.bind_vertex_buffer(0, *verticesBuf, 0, vuk::Packed{vuk::Format::eR32G32B32Sfloat})
				.bind_vertex_buffer(1, *normalsBuf, 1, vuk::Packed{vuk::Format::eR32G32B32Sfloat})
				.bind_vertex_buffer(2, *colorsBuf, 2, vuk::Packed{vuk::Format::eR16G16B16A16Unorm})
				.bind_index_buffer(*indicesBuf, vuk::IndexType::eUint16)
				.bind_storage_buffer(0, 1, instancesBuf)
				.bind_sampled_image(0, 3, *env, envSampler)
				.bind_graphics_pipeline("object");
			command_buffer.draw_indexed_indirect(commands.size(), commandsBuf);
		}
	});
	rg.add_pass({ // Cubemap draw
		.auxiliary_order = 0.0f,
		.resources = {"object_color"_image(vuk::eColorWrite), "object_depth"_image(vuk::eDepthStencilRW)},
		.execute = [this, worldBuf](vuk::CommandBuffer& command_buffer) {
			auto envSampler = vuk::SamplerCreateInfo{
				.magFilter = vuk::Filter::eLinear,
				.minFilter = vuk::Filter::eLinear,
				.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
				.addressModeV = vuk::SamplerAddressMode::eClampToEdge,
			};
			command_buffer.set_primitive_topology(vuk::PrimitiveTopology::eTriangleStrip);
			command_buffer
				.set_viewport(0, vuk::Rect2D::framebuffer())
				.set_scissor(0, vuk::Rect2D::framebuffer())
				.bind_uniform_buffer(0, 0, worldBuf)
				.bind_sampled_image(0, 1, *env, envSampler)
				.bind_graphics_pipeline("cubemap");
			command_buffer.draw(14, 1, 0, 0);
			command_buffer.set_primitive_topology(vuk::PrimitiveTopology::eTriangleList);
		}
	});
	rg.add_pass({ // Bloom threshold
		.auxiliary_order = 0.0f,
		.resources = {vuk::Resource(std::string_view(bloomNames[0]), vuk::Resource::Type::eImage, vuk::eColorWrite), "object_resolved"_image(vuk::eFragmentSampled)},
		.execute = [&bloomSizes](vuk::CommandBuffer& command_buffer) {
			command_buffer
				.set_viewport(0, vuk::Rect2D::absolute({}, bloomSizes[0].extent))
				.set_scissor(0, vuk::Rect2D::absolute({}, bloomSizes[0].extent))
				.bind_sampled_image(0, 0, "object_resolved", {})
				.bind_graphics_pipeline("bloom_threshold");
			command_buffer.draw(3, 1, 0, 0);
		},
	});
	for (auto i = 1u; i < BloomDepth; i += 1) {
		rg.add_pass({ // Bloom downscale
			.auxiliary_order = 0.0f,
			.resources = {vuk::Resource(std::string_view(bloomNames[i]), vuk::Resource::Type::eImage, vuk::eColorWrite),
			              vuk::Resource(std::string_view(bloomNames[i-1]), vuk::Resource::Type::eImage, vuk::eFragmentSampled)},
			.execute = [i, &bloomSizes, &bloomNames](vuk::CommandBuffer& command_buffer) {
				command_buffer
					.set_viewport(0, vuk::Rect2D::absolute({}, bloomSizes[i].extent))
					.set_scissor(0, vuk::Rect2D::absolute({}, bloomSizes[i].extent))
					.bind_sampled_image(0, 0, std::string_view(bloomNames[i-1]), {
						.magFilter = vuk::Filter::eLinear,
						.minFilter = vuk::Filter::eLinear,
						.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
						.addressModeV = vuk::SamplerAddressMode::eClampToEdge})
					.bind_graphics_pipeline("bloom_blur_down");
				auto* stepSize = command_buffer.map_scratch_uniform_binding<f32>(0, 1);
				*stepSize = 1.0f;
				command_buffer.draw(3, 1, 0, 0);
			},
		});
	}
	for (auto i = BloomDepth - 2; i < BloomDepth; i -= 1) {
		rg.add_pass({ // Bloom upscale
			.auxiliary_order = 1.0f,
			.resources = {vuk::Resource(std::string_view(bloomNames[i]), vuk::Resource::Type::eImage, vuk::eColorWrite),
			              vuk::Resource(std::string_view(bloomNames[i+1]), vuk::Resource::Type::eImage, vuk::eFragmentSampled)},
			.execute = [i, &bloomSizes, &bloomNames](vuk::CommandBuffer& command_buffer) {
				command_buffer
					.set_viewport(0, vuk::Rect2D::absolute({}, bloomSizes[i].extent))
					.set_scissor(0, vuk::Rect2D::absolute({}, bloomSizes[i].extent))
					.bind_sampled_image(0, 0, std::string_view(bloomNames[i+1]), {
						.magFilter = vuk::Filter::eLinear,
						.minFilter = vuk::Filter::eLinear,
						.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
						.addressModeV = vuk::SamplerAddressMode::eClampToEdge})
					.bind_graphics_pipeline("bloom_blur_up");
				auto* stepSize = command_buffer.map_scratch_uniform_binding<f32>(0, 1);
				*stepSize = 0.5f;
				command_buffer.draw(3, 1, 0, 0);
			},
		});
	}
	rg.add_pass({ // Swapchain blit
		.auxiliary_order = 1.0f,
		.resources = {"swapchain"_image(vuk::eColorWrite), "object_resolved"_image(vuk::eFragmentSampled),
		              vuk::Resource(std::string_view(bloomNames[0]), vuk::Resource::Type::eImage, vuk::eFragmentSampled)},
		.execute = [&bloomNames](vuk::CommandBuffer& command_buffer) {
			command_buffer
				.set_viewport(0, vuk::Rect2D::framebuffer())
				.set_scissor(0, vuk::Rect2D::framebuffer())
				.bind_sampled_image(0, 0, "object_resolved", {})
				.bind_sampled_image(0, 1, std::string_view(bloomNames[0]), {})
				.bind_graphics_pipeline("swapchain_blit");
			command_buffer.draw(3, 1, 0, 0);
		},
	});

#if IMGUI
	ImGui::Render();
	ImGui_ImplVuk_Render(ptc, rg, "swapchain", "swapchain", imguiData, ImGui::GetDrawData());
#endif //IMGUI

	rg.attach_managed("object_color", vuk::Format::eR16G16B16A16Sfloat, swapchainSize, vuk::Samples::e4, vuk::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
	rg.attach_managed("object_depth", vuk::Format::eD32Sfloat, swapchainSize, vuk::Samples::e4, vuk::ClearDepthStencil{1.0f, 0});
	rg.attach_managed("object_resolved", vuk::Format::eR16G16B16A16Sfloat, swapchainSize, vuk::Samples::e1, vuk::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
	rg.resolve_resource_into("object_resolved", "object_color");
	for (auto i = 0u; i < BloomDepth; i += 1) {
		rg.attach_managed(std::string_view(bloomNames[i]), vuk::Format::eR16G16B16A16Sfloat,
			bloomSizes[i], vuk::Samples::e1, vuk::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
	}
	rg.attach_swapchain("swapchain", swapchain, vuk::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
	auto erg = std::move(rg).link(ptc);

	// Acquire swapchain image
	auto presentSem = ptc.acquire_semaphore();
	auto swapchainImageIndex = [&, this] {
		while (true) {
			u32 result;
			VkResult error = vkAcquireNextImageKHR(device.device, swapchain->swapchain, UINT64_MAX, presentSem, VK_NULL_HANDLE, &result);
			if (error == VK_SUCCESS || error == VK_SUBOPTIMAL_KHR)
				return result;
			if (error == VK_ERROR_OUT_OF_DATE_KHR)
				refreshSwapchain();
			else
				throw std::runtime_error(fmt::format("Unable to acquire swapchain image: error {}", error));
		}
	}();

	// Build the rendergraph
	auto commandBuffer = erg.execute(ptc, {{swapchain, swapchainImageIndex}});

	// Submit the command buffer
	auto renderSem = ptc.acquire_semaphore();
	auto waitStage = VkPipelineStageFlags(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	auto submitInfo = VkSubmitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &presentSem,
		.pWaitDstStageMask = &waitStage,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &renderSem,
	};
	context->submit_graphics(submitInfo, ptc.acquire_fence());

	auto presentInfo = VkPresentInfoKHR{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &renderSem,
		.swapchainCount = 1,
		.pSwapchains = &swapchain->swapchain,
		.pImageIndices = &swapchainImageIndex,
	};
	auto result = vkQueuePresentKHR(context->graphics_queue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		refreshSwapchain();
	else if (result != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Unable to present to the screen: error {}", result));

	// Clean up
#if IMGUI
	ImGui::NewFrame();
#endif //IMGUI
}

auto Engine::createSwapchain(VkSwapchainKHR old) -> vuk::Swapchain {
	auto vkbswapchainResult = vkb::SwapchainBuilder(device)
		.set_old_swapchain(old)
		.set_image_usage_flags(VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build();
	if (!vkbswapchainResult)
		throw std::runtime_error(fmt::format("Failed to create the swapchain: {}", vkbswapchainResult.error().message()));
	auto vkbswapchain = vkbswapchainResult.value();

	auto vuksw = vuk::Swapchain{
		.swapchain = vkbswapchain.swapchain,
		.surface = surface,
		.format = vuk::Format(vkbswapchain.image_format),
		.extent = { vkbswapchain.extent.width, vkbswapchain.extent.height },
	};
	auto imgs = vkbswapchain.get_images();
	auto ivs = vkbswapchain.get_image_views();
	for (auto& img: *imgs)
		vuksw.images.emplace_back(img);
	for (auto& iv: *ivs)
		vuksw.image_views.emplace_back().payload = iv;
	return vuksw;
}

void Engine::refreshSwapchain() {
	for (auto iv: swapchain->image_views)
		context->enqueue_destroy(iv);
	auto newSwapchain = context->add_swapchain(createSwapchain(swapchain->swapchain));
	context->remove_swapchain(swapchain);
	swapchain = newSwapchain;
#if IMGUI
	ImGui::GetIO().DisplaySize = ImVec2(f32(swapchain->extent.width), f32(swapchain->extent.height));
#endif //IMGUI
}

}

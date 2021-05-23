#include "gfx/engine.hpp"

#include "config.hpp"

#include <stdexcept>
#include <cstring>
#include <cassert>
#include "VkBootstrap.h"
#include "GLFW/glfw3.h"
#include "fmt/core.h"
#include "volk.h"
#include "vuk/CommandBuffer.hpp"
#include "vuk/RenderGraph.hpp"
#include "base/math.hpp"
#include "base/log.hpp"
#include "gfx/pipelines.hpp"
#include "gfx/indirect.hpp"
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
		.enable_layer("VK_LAYER_KHRONOS_validation")
		.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT)
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
		.multiDrawIndirect = VK_TRUE,
	};
	auto physicalDeviceVulkan12Features = VkPhysicalDeviceVulkan12Features{
		.descriptorBindingPartiallyBound = VK_TRUE,
		.descriptorBindingVariableDescriptorCount = VK_TRUE,
		.runtimeDescriptorArray = VK_TRUE,
		.hostQueryReset = VK_TRUE,
	};
	auto physicalDeviceSelectorResult = vkb::PhysicalDeviceSelector(instance)
		.set_surface(surface)
		.set_minimum_version(1, 2)
		.set_required_features(physicalDeviceFeatures)
		.set_required_features_12(physicalDeviceVulkan12Features)
		.select();
	if (!physicalDeviceSelectorResult)
		throw std::runtime_error(fmt::format("Failed to find a suitable GPU for Vulkan: {}", physicalDeviceSelectorResult.error().message()));
	auto physicalDevice = physicalDeviceSelectorResult.value();

	// Create device
	auto deviceResult = vkb::DeviceBuilder(physicalDevice).build();
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
	ibl.reset();
	sky.reset();
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
	meshes.addGltf(name, model);
}

void Engine::uploadAssets() {
	auto ifc = context->begin();
	auto ptc = ifc.begin();

	createPipelines(*context);

#if IMGUI
	imguiData = ImGui_ImplVuk_Init(ptc);
	ImGui::GetIO().DisplaySize = ImVec2(f32(swapchain->extent.width), f32(swapchain->extent.height));
#endif //IMGUI

	// Upload mesh buffers
	verticesBuf = ptc.create_buffer(vuk::MemoryUsage::eGPUonly,
		vuk::BufferUsageFlagBits::eVertexBuffer, std::span<vec3>(meshes.vertices)).first;
	normalsBuf = ptc.create_buffer(vuk::MemoryUsage::eGPUonly,
		vuk::BufferUsageFlagBits::eVertexBuffer, std::span<vec3>(meshes.normals)).first;
	colorsBuf = ptc.create_buffer(vuk::MemoryUsage::eGPUonly,
		vuk::BufferUsageFlagBits::eVertexBuffer, std::span<u16vec4>(meshes.colors)).first;
	indicesBuf = ptc.create_buffer(vuk::MemoryUsage::eGPUonly,
		vuk::BufferUsageFlagBits::eIndexBuffer, std::span<u16>(meshes.indices)).first;

	// Create pipeline components
	sky = Sky(*context);
	ibl = IBLMap(*context, ptc);

	// Finalize uploads
	ptc.wait_all_transfers();

	// Begin imgui frame so that first-frame calls succeed
#if IMGUI
	ImGui::NewFrame();
#endif //IMGUI
}

void Engine::enqueue(ID mesh, std::span<Instance const> _instances) {
	instances.addInstances(mesh, _instances);
}

void Engine::render() {
	// Prepare per-frame data
	auto viewport = uvec2(swapchain->extent.width, swapchain->extent.height);
	auto rawview = camera.transform();
	// auto zFlip = make_scale({-1.0f, -1.0f, 1.0f});
	world.projection = infinitePerspective(VerticalFov, f32(viewport.x) / f32(viewport.y), NearPlane);
	// Reverse-Z
	world.projection[2][2] = 0.0f;
	world.projection[3][2] *= -1.0f;
	// world.view = zFlip * rawview;
	world.view = rawview;
	world.viewProjection = world.projection * world.view;
	world.viewProjectionInverse = inverse(world.viewProjection);
	world.viewportSize = {swapchain->extent.width, swapchain->extent.height};
	world.cameraPos = camera.position;
	auto swapchainSize = vuk::Dimension2D::absolute(swapchain->extent);

	static auto sunPitch = radians(7.2f);
	static auto sunYaw = radians(30.0f);
#if IMGUI
	ImGui::SliderAngle("Sun pitch", &sunPitch, -8.0f, 60.0f, "%.1f deg", ImGuiSliderFlags_NoRoundToFormat);
	ImGui::SliderAngle("Sun yaw", &sunYaw, -180.0f, 180.0f, nullptr, ImGuiSliderFlags_NoRoundToFormat);
#endif //IMGUI
	// sunPitch = radians(15.0f - glfwGetTime() / 2.0);
	world.sunDirection = vec3(1.0f, 0.0f, 0.0f);
	world.sunDirection = glm::mat3(make_rotate(sunPitch, {0.0f, -1.0f, 0.0f})) * world.sunDirection;
	world.sunDirection = glm::mat3(make_rotate(sunYaw, {0.0f, 0.0f, 1.0f})) * world.sunDirection;
	static auto scattering = 1.0f;
#if IMGUI
	ImGui::SliderFloat("Multiple scattering", &scattering, 0.01f, 100.0f, nullptr, ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
#endif //IMGUI
	world.multiScatteringFactor = scattering;
	static auto sunIlluminance = 1.0f;
#if IMGUI
	ImGui::SliderFloat("Sun illuminance", &sunIlluminance, 0.01f, 100.0f, nullptr, ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
#endif //IMGUI
	world.sunIlluminance = vec3(sunIlluminance);

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
	auto indirect = Indirect::createBuffers(ptc, meshes, instances);

	// Set up the rendergraph
	auto rg = vuk::RenderGraph();

	float const EarthRayleighScaleHeight = 8.0f;
	float const EarthMieScaleHeight = 1.2f;
	auto const MieScattering = vec3{0.003996f, 0.003996f, 0.003996f};
	auto const MieExtinction = vec3{0.004440f, 0.004440f, 0.004440f};

	auto atmosphere = Sky::AtmosphereParams{
		.BottomRadius = 6360.0f,
		.TopRadius = 6460.0f,
		.RayleighDensityExpScale = -1.0f / EarthRayleighScaleHeight,
		.RayleighScattering = {0.005802f, 0.013558f, 0.033100f},
		.MieDensityExpScale = -1.0f / EarthMieScaleHeight,
		.MieScattering = MieScattering,
		.MieExtinction = MieExtinction,
		.MieAbsorption = max(MieExtinction - MieScattering, vec3(0.0f)),
		.MiePhaseG = 0.8f,
		.AbsorptionDensity0LayerWidth = 25.0f,
		.AbsorptionDensity0ConstantTerm = -2.0f / 3.0f,
		.AbsorptionDensity0LinearTerm = 1.0f / 15.0f,
		.AbsorptionDensity1ConstantTerm = 8.0f / 3.0f,
		.AbsorptionDensity1LinearTerm = -1.0f / 15.0f,
		.AbsorptionExtinction = {0.000650f, 0.001881f, 0.000085f},
		.GroundAlbedo = {0.0f, 0.0f, 0.0f},
	};
	rg.append(sky->generateAtmosphereModel(atmosphere, worldBuf, ptc));
	rg.append(sky->drawCubemap(atmosphere, "ibl_map_unfiltered", ibl->BaseSize, worldBuf, ptc));
	rg.append(ibl->filter());
	rg.add_pass({
		.name = "Frustum culling",
		.resources = {
			"commands"_buffer(vuk::eComputeRW),
			"instances"_buffer(vuk::eComputeRead),
			"instances_culled"_buffer(vuk::eComputeWrite),
		},
		.execute = [this, &indirect](vuk::CommandBuffer& cmd) {
			auto commandsBuf = cmd.get_resource_buffer("commands");
			auto instancesBuf = cmd.get_resource_buffer("instances");
			auto instancesCulledBuf = cmd.get_resource_buffer("instances_culled");
			cmd.bind_storage_buffer(0, 0, commandsBuf)
			   .bind_storage_buffer(0, 1, instancesBuf)
			   .bind_storage_buffer(0, 2, instancesCulledBuf)
			   .bind_compute_pipeline("cull");
			struct CullData {
				mat4 view;
				vec4 frustum;
				u32 instancesCount;
			};
			auto* cullData = cmd.map_scratch_uniform_binding<CullData>(0, 3);
			*cullData = CullData{
				.view = world.view,
				.frustum = [this] {
					auto projectionT = transpose(world.projection);
					vec4 frustumX = projectionT[3] + projectionT[0];
					vec4 frustumY = projectionT[3] + projectionT[1];
					frustumX /= length(vec3(frustumX));
					frustumY /= length(vec3(frustumY));
					return vec4(frustumX.x, frustumX.z, frustumY.y, frustumY.z);
				}(),
				.instancesCount = u32(indirect.instancesCount),
			};
			cmd.dispatch_invocations(indirect.instancesCount);
		},
	});
	rg.add_pass({
		.name = "Z-prepass",
		.resources = {
			"commands"_buffer(vuk::eIndirectRead),
			"instances_culled"_buffer(vuk::eVertexRead),
			"object_depth"_image(vuk::eDepthStencilRW),
		},
		.execute = [this, worldBuf, &indirect](vuk::CommandBuffer& cmd) {
			auto commandsBuf = cmd.get_resource_buffer("commands");
			auto instancesBuf = cmd.get_resource_buffer("instances_culled");
			cmd.set_viewport(0, vuk::Rect2D::framebuffer())
			   .set_scissor(0, vuk::Rect2D::framebuffer())
			   .bind_uniform_buffer(0, 0, worldBuf)
			   .bind_vertex_buffer(0, *verticesBuf, 0, vuk::Packed{vuk::Format::eR32G32B32Sfloat})
			   .bind_index_buffer(*indicesBuf, vuk::IndexType::eUint16)
			   .bind_storage_buffer(0, 1, instancesBuf)
			   .bind_graphics_pipeline("z_prepass");
			cmd.draw_indexed_indirect(indirect.commandsCount, commandsBuf, sizeof(Indirect::Command));
		},
	});
	rg.add_pass({
		.name = "Object drawing",
		.resources = {
			"commands"_buffer(vuk::eIndirectRead),
			"instances_culled"_buffer(vuk::eVertexRead),
			"ibl_map_filtered"_image(vuk::eFragmentSampled),
			"sky_aerial_perspective"_image(vuk::eFragmentSampled),
			"object_color"_image(vuk::eColorWrite),
			"object_depth"_image(vuk::eDepthStencilRW),
		},
		.execute = [this, worldBuf, &indirect](vuk::CommandBuffer& cmd) {
			auto cubeSampler = vuk::SamplerCreateInfo{
				.magFilter = vuk::Filter::eLinear,
				.minFilter = vuk::Filter::eLinear,
				.mipmapMode = vuk::SamplerMipmapMode::eLinear,
			};
			auto aerialSampler = vuk::SamplerCreateInfo{
				.magFilter = vuk::Filter::eLinear,
				.minFilter = vuk::Filter::eLinear,
				.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
				.addressModeV = vuk::SamplerAddressMode::eClampToEdge,
				.addressModeW = vuk::SamplerAddressMode::eClampToEdge,
			};
			auto commandsBuf = cmd.get_resource_buffer("commands");
			auto instancesBuf = cmd.get_resource_buffer("instances_culled");
			cmd.set_viewport(0, vuk::Rect2D::framebuffer())
			   .set_scissor(0, vuk::Rect2D::framebuffer())
			   .bind_uniform_buffer(0, 0, worldBuf)
			   .bind_vertex_buffer(0, *verticesBuf, 0, vuk::Packed{vuk::Format::eR32G32B32Sfloat})
			   .bind_vertex_buffer(1, *normalsBuf, 1, vuk::Packed{vuk::Format::eR32G32B32Sfloat})
			   .bind_vertex_buffer(2, *colorsBuf, 2, vuk::Packed{vuk::Format::eR16G16B16A16Unorm})
			   .bind_index_buffer(*indicesBuf, vuk::IndexType::eUint16)
			   .bind_storage_buffer(0, 1, instancesBuf)
			   .bind_sampled_image(0, 3, "ibl_map_filtered", cubeSampler)
			   .bind_sampled_image(0, 4, "sky_aerial_perspective", aerialSampler)
			   .bind_graphics_pipeline("object");
			cmd.draw_indexed_indirect(indirect.commandsCount, commandsBuf, sizeof(Indirect::Command));
		},
	});
	rg.append(sky->draw(atmosphere, "object_color", "object_depth", worldBuf, ptc));
	rg.resolve_resource_into("object_resolved", "object_color");
	rg.add_pass({
		.name = "Tonemapping",
		.resources = {
			"object_resolved"_image(vuk::eFragmentSampled),
			"swapchain"_image(vuk::eColorWrite),
		},
		.execute = [](vuk::CommandBuffer& cmd) {
			cmd.set_viewport(0, vuk::Rect2D::framebuffer())
			   .set_scissor(0, vuk::Rect2D::framebuffer())
			   .bind_sampled_image(0, 0, "object_resolved", {})
			   .bind_graphics_pipeline("tonemap");
			cmd.draw(3, 1, 0, 0);
		},
	});

#if IMGUI
	ImGui::Render();
	ImGui_ImplVuk_Render(ptc, rg, "swapchain", "swapchain", imguiData, ImGui::GetDrawData());
#endif //IMGUI

	rg.attach_buffer("commands", indirect.commands, vuk::eTransferDst, {});
	rg.attach_buffer("instances", indirect.instances, vuk::eTransferDst, {});
	rg.attach_buffer("instances_culled", indirect.instancesCulled, {}, {});
	rg.attach_managed("object_color", vuk::Format::eR16G16B16A16Sfloat, swapchainSize, vuk::Samples::e4, vuk::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
	rg.attach_managed("object_depth", vuk::Format::eD32Sfloat, swapchainSize, vuk::Samples::e4, vuk::ClearDepthStencil{0.0f, 0});
	rg.attach_managed("object_resolved", vuk::Format::eR16G16B16A16Sfloat, swapchainSize, vuk::Samples::e1, vuk::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
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
	instances.clear();
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

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
	sky.reset();
	cubemapPds.reset();
	cubemapMips.clear();
	cubemapBase.reset();
	cubemap.reset();
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
		vuk::BufferUsageFlagBits::eVertexBuffer, std::span(meshes.vertices)).first;
	normalsBuf = ptc.create_buffer(vuk::MemoryUsage::eGPUonly,
		vuk::BufferUsageFlagBits::eVertexBuffer, std::span(meshes.normals)).first;
	colorsBuf = ptc.create_buffer(vuk::MemoryUsage::eGPUonly,
		vuk::BufferUsageFlagBits::eVertexBuffer, std::span(meshes.colors)).first;
	indicesBuf = ptc.create_buffer(vuk::MemoryUsage::eGPUonly,
		vuk::BufferUsageFlagBits::eIndexBuffer, std::span(meshes.indices)).first;

	// Create cubemap and its views
	auto cubemapMipCount = mipmapCount(CubeMapSize);
	cubemap = context->allocate_texture(vuk::ImageCreateInfo{
		.flags = vuk::ImageCreateFlagBits::eCubeCompatible,
		.format = vuk::Format::eR16G16B16A16Sfloat,
		.extent = {CubeMapSize, CubeMapSize, 1},
		.mipLevels = cubemapMipCount,
		.arrayLayers = 6,
		.usage = vuk::ImageUsageFlagBits::eStorage | vuk::ImageUsageFlagBits::eSampled,
	});
	cubemap->view = ptc.create_image_view(vuk::ImageViewCreateInfo{
		.image = cubemap->image.get(),
		.viewType = vuk::ImageViewType::eCube,
		.format = cubemap->format,
		.subresourceRange = vuk::ImageSubresourceRange{
			.aspectMask = vuk::ImageAspectFlagBits::eColor,
			.levelCount = VK_REMAINING_MIP_LEVELS,
			.layerCount = 6,
		},
	});
	cubemapBase = ptc.create_image_view(vuk::ImageViewCreateInfo{
		.image = cubemap->image.get(),
		.viewType = vuk::ImageViewType::e2DArray,
		.format = cubemap->format,
		.subresourceRange = vuk::ImageSubresourceRange{
			.aspectMask = vuk::ImageAspectFlagBits::eColor,
			.layerCount = 6,
		},
	});
	cubemapPds = ptc.create_persistent_descriptorset(*context->get_named_compute_pipeline("cubemip"), 0, 16);
	for (auto i = 0u; i < cubemapMipCount; i += 1) {
		auto& view = cubemapMips.emplace_back(ptc.create_image_view(vuk::ImageViewCreateInfo{
			.image = cubemap->image.get(),
			.viewType = vuk::ImageViewType::e2DArray,
			.format = cubemap->format,
			.subresourceRange = vuk::ImageSubresourceRange{
				.aspectMask = vuk::ImageAspectFlagBits::eColor,
				.baseMipLevel = i,
				.levelCount = 1,
				.layerCount = 6,
			},
		}));
		cubemapPds->update_storage_image(ptc, 0, i, view.get());
	}
	ptc.commit_persistent_descriptorset(cubemapPds.get());

	sky = Sky(*context);

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

void Engine::enqueue(ID mesh, std::span<Instance const> _instances) {
	instances.addInstances(mesh, _instances);
}

void Engine::render() {
	// Prepare per-frame data
	auto viewport = glm::uvec2(swapchain->extent.width, swapchain->extent.height);
	auto rawview = glm::lookAt(camera.eye, camera.center, camera.up);
	auto yFlip = make_scale({-1.0f, -1.0f, 1.0f});
	world.projection = glm::infinitePerspective(VerticalFov, f32(viewport.x) / f32(viewport.y), NearPlane);
//	world.projection = glm::perspective(VerticalFov, f32(viewport.x) / f32(viewport.y), NearPlane, 1000.0f);
	world.view = yFlip * rawview;
	world.viewProjection = world.projection * world.view;

	constexpr auto BloomDepth = 6u;
	auto bloomNames = std::array<std::string, BloomDepth>();
	for (auto i = 0u; i < BloomDepth; i += 1)
		bloomNames[i] = "bloom"s + std::to_string(i);
	auto swapchainSize = vuk::Dimension2D::absolute(swapchain->extent);
	auto bloomSizes = std::array<vuk::Dimension2D, BloomDepth>();
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

	auto cubemipAtomic = ptc.allocate_scratch_buffer(vuk::MemoryUsage::eGPUonly,
		vuk::BufferUsageFlagBits::eStorageBuffer, sizeof(glm::vec4) * 256 * 6 + sizeof(u32) * 6, alignof(glm::vec4));

	// Upload indirect buffers
	auto indirect = Indirect::createBuffers(ptc, meshes, instances);

	// Set up the rendergraph
	auto rg = vuk::RenderGraph();

	float const EarthRayleighScaleHeight = 8.0f;
	float const EarthMieScaleHeight = 1.2f;
	auto const MieScattering = glm::vec3{0.003996f, 0.003996f, 0.003996f};
	auto const MieExtinction = glm::vec3{0.004440f, 0.004440f, 0.004440f};

	/*rg.append(sky->generateAtmosphereModel(Sky::AtmosphereParams{
		.BottomRadius = 6360.0f,
		.TopRadius = 6460.0f,
		.RayleighDensityExpScale = -1.0f / EarthRayleighScaleHeight,
		.RayleighScattering = {0.005802f, 0.013558f, 0.033100f},
		.MieDensityExpScale = -1.0f / EarthMieScaleHeight,
		.MieScattering = MieScattering,
		.MieExtinction = MieExtinction,
		.MieAbsorption = glm::max(MieExtinction - MieScattering, glm::vec3(0.0f)),
		.MiePhaseG = 0.8f,
		.AbsorptionDensity0LayerWidth = 25.0f,
		.AbsorptionDensity0ConstantTerm = -2.0f / 3.0f,
		.AbsorptionDensity0LinearTerm = 1.0f / 15.0f,
		.AbsorptionDensity1ConstantTerm = 8.0f / 3.0f,
		.AbsorptionDensity1LinearTerm = -1.0f / 15.0f,
		.AbsorptionExtinction = {0.000650f, 0.001881f, 0.000085f},
		.GroundAlbedo = {0.0f, 0.0f, 0.0f},
	}, ptc, {swapchain->extent.width, swapchain->extent.height}, camera.eye, world.viewProjection));*/

	rg.add_pass({
		.name = "Sky generation",
		.resources = {
//			"sky_sky_view"_image(vuk::eComputeRead),
			"cubemap"_image(vuk::eComputeWrite),
		},
		.execute = [](vuk::CommandBuffer& cmd) {
			cmd.bind_storage_image(0, 0, "cubemap")
			   .bind_compute_pipeline("cubemap");
			auto* sides = cmd.map_scratch_uniform_binding<std::array<glm::mat4, 6>>(0, 1);
			*sides = std::to_array<glm::mat4>({
			glm::mat3{
				0.0f, 0.0f, -1.0f,
				0.0f, -1.0f, 0.0f,
				1.0f, 0.0f, 0.0f,
			}, glm::mat3{
				0.0f, 0.0f, 1.0f,
				0.0f, -1.0f, 0.0f,
				-1.0f, 0.0f, 0.0f,
			}, glm::mat3{
				1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f,
				0.0f, 1.0f, 0.0f,
			}, glm::mat3{
				1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, -1.0f,
				0.0f, -1.0f, 0.0f,
			}, glm::mat3{
				1.0f, 0.0f, 0.0f,
				0.0f, -1.0f, 0.0f,
				0.0f, 0.0f, 1.0f,
			}, glm::mat3{
				-1.0f, 0.0f, 0.0f,
				0.0f, -1.0f, 0.0f,
				0.0f, 0.0f, -1.0f,
			}});
			cmd.dispatch_invocations(CubeMapSize, CubeMapSize, 6);
		},
	});
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
				glm::mat4 view;
				glm::vec4 frustum;
				u32 instancesCount;
			};
			auto* cullData = cmd.map_scratch_uniform_binding<CullData>(0, 3);
			*cullData = CullData{
				.view = world.view,
				.frustum = [this] {
					glm::vec4 frustumX = world.viewProjection[3] + world.viewProjection[0];
					glm::vec4 frustumY = world.viewProjection[3] + world.viewProjection[1];
					frustumX /= glm::length(glm::vec3(frustumX));
					frustumY /= glm::length(glm::vec3(frustumY));
					return glm::vec4(frustumX.x, frustumX.z, frustumY.y, frustumY.z);
				}(),
				.instancesCount = u32(indirect.instancesCount),
			};
			cmd.dispatch_invocations(indirect.instancesCount);
		},
	});
	rg.add_pass({
		.name = "Generate cubemap mips",
		.resources = {
			"cubemap"_image(vuk::eComputeRW),
		},
		.execute = [this, &cubemipAtomic](vuk::CommandBuffer& cmd) {
			cmd.bind_persistent(0, cubemapPds.get())
			   .bind_storage_buffer(1, 0, cubemipAtomic)
			   .bind_sampled_image(1, 1, cubemapBase.get(), vuk::SamplerCreateInfo{
			   	.magFilter = vuk::Filter::eLinear,
			   	.minFilter = vuk::Filter::eLinear,
			   }, vuk::ImageLayout::eGeneral)
			   .bind_compute_pipeline("cubemip");
			cmd.dispatch_invocations(CubeMapSize / 4, CubeMapSize / 4, 6);
		},
	});
	rg.add_pass({
		.name = "Z-prepass",
		.auxiliary_order = 1.0f,
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
		.auxiliary_order = 2.0f,
		.resources = {
			"commands"_buffer(vuk::eIndirectRead),
			"instances_culled"_buffer(vuk::eVertexRead),
			"cubemap"_image(vuk::eFragmentSampled),
			"object_color"_image(vuk::eColorWrite),
			"object_depth"_image(vuk::eDepthStencilRW),
		},
		.execute = [this, worldBuf, &indirect](vuk::CommandBuffer& cmd) {
			auto cubeSampler = vuk::SamplerCreateInfo{
				.magFilter = vuk::Filter::eLinear,
				.minFilter = vuk::Filter::eLinear,
				.mipmapMode = vuk::SamplerMipmapMode::eLinear,
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
			   .bind_sampled_image(0, 3, "cubemap", cubeSampler)
			   .bind_graphics_pipeline("object");
			cmd.draw_indexed_indirect(indirect.commandsCount, commandsBuf, sizeof(Indirect::Command));
		},
	});
	rg.add_pass({
		.name = "Sky drawing",
		.auxiliary_order = 3.0f,
		.resources = {
			"cubemap"_image(vuk::eFragmentSampled),
			"object_color"_image(vuk::eColorWrite),
			"object_depth"_image(vuk::eDepthStencilRW),
		},
		.execute = [this, worldBuf](vuk::CommandBuffer& cmd) {
			auto cubeSampler = vuk::SamplerCreateInfo{
				.magFilter = vuk::Filter::eLinear,
				.minFilter = vuk::Filter::eLinear,
				.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
				.addressModeV = vuk::SamplerAddressMode::eClampToEdge,
			};
			cmd.set_primitive_topology(vuk::PrimitiveTopology::eTriangleStrip);
			cmd.set_viewport(0, vuk::Rect2D::framebuffer())
			   .set_scissor(0, vuk::Rect2D::framebuffer())
			   .bind_uniform_buffer(0, 0, worldBuf)
			   .bind_sampled_image(0, 1, *cubemap, cubeSampler)
			   .bind_graphics_pipeline("sky");
			cmd.draw(14, 1, 0, 0);
			cmd.set_primitive_topology(vuk::PrimitiveTopology::eTriangleList);
		},
	});
	rg.add_pass({
		.name = "Bloom threshold pass",
		.resources = {
			vuk::Resource(std::string_view(bloomNames[0]), vuk::Resource::Type::eImage, vuk::eColorWrite),
			"object_resolved"_image(vuk::eFragmentSampled),
		},
		.execute = [&bloomSizes](vuk::CommandBuffer& cmd) {
			cmd.set_viewport(0, vuk::Rect2D::absolute({}, bloomSizes[0].extent))
			   .set_scissor(0, vuk::Rect2D::absolute({}, bloomSizes[0].extent))
			   .bind_sampled_image(0, 0, "object_resolved", {})
			   .bind_graphics_pipeline("bloom_threshold");
			cmd.draw(3, 1, 0, 0);
		},
	});
	for (auto i = 1u; i < BloomDepth; i += 1) {
		rg.add_pass({
			.name = "Bloom downscale pass",
			.resources = {
				vuk::Resource(std::string_view(bloomNames[i]), vuk::Resource::Type::eImage, vuk::eColorWrite),
			    vuk::Resource(std::string_view(bloomNames[i-1]), vuk::Resource::Type::eImage, vuk::eFragmentSampled),
			},
			.execute = [i, &bloomSizes, &bloomNames](vuk::CommandBuffer& cmd) {
				cmd.set_viewport(0, vuk::Rect2D::absolute({}, bloomSizes[i].extent))
				   .set_scissor(0, vuk::Rect2D::absolute({}, bloomSizes[i].extent))
				   .bind_sampled_image(0, 0, std::string_view(bloomNames[i - 1]), {
					   .magFilter = vuk::Filter::eLinear,
					   .minFilter = vuk::Filter::eLinear,
					   .addressModeU = vuk::SamplerAddressMode::eClampToEdge,
					   .addressModeV = vuk::SamplerAddressMode::eClampToEdge})
				   .bind_graphics_pipeline("bloom_blur_down");
				auto* stepSize = cmd.map_scratch_uniform_binding<f32>(0, 1);
				*stepSize = 1.0f;
				cmd.draw(3, 1, 0, 0);
			},
		});
	}
	for (auto i = BloomDepth - 2; i < BloomDepth; i -= 1) {
		rg.add_pass({
			.name = "Bloom upscale pass",
			.auxiliary_order = 1.0f,
			.resources = {
				vuk::Resource(std::string_view(bloomNames[i]), vuk::Resource::Type::eImage, vuk::eColorWrite),
				vuk::Resource(std::string_view(bloomNames[i+1]), vuk::Resource::Type::eImage, vuk::eFragmentSampled),
			},
			.execute = [i, &bloomSizes, &bloomNames](vuk::CommandBuffer& cmd) {
				cmd.set_viewport(0, vuk::Rect2D::absolute({}, bloomSizes[i].extent))
				   .set_scissor(0, vuk::Rect2D::absolute({}, bloomSizes[i].extent))
				   .bind_sampled_image(0, 0, std::string_view(bloomNames[i + 1]), {
					   .magFilter = vuk::Filter::eLinear,
					   .minFilter = vuk::Filter::eLinear,
					   .addressModeU = vuk::SamplerAddressMode::eClampToEdge,
					   .addressModeV = vuk::SamplerAddressMode::eClampToEdge})
					.bind_graphics_pipeline("bloom_blur_up");
				auto* stepSize = cmd.map_scratch_uniform_binding<f32>(0, 1);
				*stepSize = 0.5f;
				cmd.draw(3, 1, 0, 0);
			},
		});
	}
	rg.add_pass({
		.name = "Tonemapping",
		.resources = {
			"swapchain"_image(vuk::eColorWrite),
			"object_resolved"_image(vuk::eFragmentSampled),
			vuk::Resource(std::string_view(bloomNames[0]), vuk::Resource::Type::eImage, vuk::eFragmentSampled),
		},
		.execute = [&bloomNames](vuk::CommandBuffer& cmd) {
			cmd.set_viewport(0, vuk::Rect2D::framebuffer())
			   .set_scissor(0, vuk::Rect2D::framebuffer())
			   .bind_sampled_image(0, 0, "object_resolved", {})
			   .bind_sampled_image(0, 1, std::string_view(bloomNames[0]), {})
			   .bind_graphics_pipeline("tonemap");
			cmd.draw(3, 1, 0, 0);
		},
	});

#if IMGUI
	ImGui::Render();
	ImGui_ImplVuk_Render(ptc, rg, "swapchain", "swapchain", imguiData, ImGui::GetDrawData());
#endif //IMGUI

	rg.attach_image("cubemap", vuk::ImageAttachment::from_texture(*cubemap), {}, {});
	rg.attach_buffer("commands", indirect.commands, vuk::eTransferDst, {});
	rg.attach_buffer("instances", indirect.instances, vuk::eTransferDst, {});
	rg.attach_buffer("instances_culled", indirect.instancesCulled, {}, {});
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

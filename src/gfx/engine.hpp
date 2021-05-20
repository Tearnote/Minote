#pragma once

#include "config.hpp"

#include <optional>
#include <span>
#include "VkBootstrap.h"
#include "volk.h"
#include "vuk/Context.hpp"
#include "base/version.hpp"
#include "base/hashmap.hpp"
#include "base/math.hpp"
#include "base/id.hpp"
#include "sys/window.hpp"
#include "gfx/instances.hpp"
#include "gfx/meshes.hpp"
#include "gfx/world.hpp"
#if IMGUI
#include "gfx/imgui.hpp"
#endif //IMGUI
#include "gfx/sky.hpp"
#include "gfx/ibl.hpp"

namespace minote::gfx {

using namespace base;

struct Engine {

	using Instance = Instances::Instance;

	explicit Engine(sys::Window& window, Version version);
	~Engine();

	void addModel(std::string_view name, std::span<char const> model);
	void uploadAssets();

	void setCamera(vec3 eye, vec3 center, vec3 up = {0.0f, 0.0f, 1.0f});
	void enqueue(ID mesh, std::span<Instance const> instances);

	void render();

	Engine(Engine const&) = delete;
	auto operator=(Engine const&) -> Engine& = delete;

private:

	vkb::Instance instance;
	VkSurfaceKHR surface;
	vkb::Device device;
	vuk::SwapChainRef swapchain;
	std::optional<vuk::Context> context;
#if IMGUI
	ImguiData imguiData;
#endif //IMGUI

	Meshes meshes;
	Instances instances;

	vuk::Unique<vuk::Buffer> verticesBuf;
	vuk::Unique<vuk::Buffer> normalsBuf;
	vuk::Unique<vuk::Buffer> colorsBuf;
	vuk::Unique<vuk::Buffer> indicesBuf;

	World world;

	struct Camera {
		vec3 eye;
		vec3 center;
		vec3 up;
	} camera;

	std::optional<Sky> sky;
	std::optional<IBLMap> ibl;

	auto createSwapchain(VkSwapchainKHR old = VK_NULL_HANDLE) -> vuk::Swapchain;
	void refreshSwapchain();

};

}

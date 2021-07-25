#pragma once

#include "config.hpp"

#include <optional>
#include <span>
#include "VkBootstrap.h"
#include "volk.h"
#include "vuk/Context.hpp"
#include "base/container/hashmap.hpp"
#include "base/util.hpp"
#include "base/math.hpp"
#include "base/id.hpp"
#include "sys/window.hpp"
#include "gfx/module/world.hpp"
#include "gfx/module/sky.hpp"
#include "gfx/module/ibl.hpp"
#include "gfx/objects.hpp"
#include "gfx/meshes.hpp"
#include "gfx/camera.hpp"
#if IMGUI
#include "gfx/imgui.hpp"
#endif //IMGUI

namespace minote::gfx {

using namespace base;
using namespace base::literals;

struct Engine {
	
	static constexpr auto VerticalFov = 45_deg;
	static constexpr auto NearPlane = 0.1_m;
	
	explicit Engine(sys::Window& window, Version version);
	~Engine();
	
	void uploadAssets();
	
	void render();
	
	Engine(Engine const&) = delete;
	auto operator=(Engine const&) -> Engine& = delete;
	
	vkb::Instance instance;
	VkSurfaceKHR surface;
	vkb::Device device;
	vuk::SwapChainRef swapchain;
	std::optional<vuk::Context> context;
#if IMGUI
	ImguiData imguiData;
#endif //IMGUI
	
	std::optional<Meshes> meshes;
	std::optional<Objects> objects;
	World world;
	Camera camera;
	
	std::optional<Atmosphere> atmosphere;
	std::optional<IBLMap> ibl;
	
	auto createSwapchain(VkSwapchainKHR old = VK_NULL_HANDLE) -> vuk::Swapchain;
	void refreshSwapchain();
	
};

}

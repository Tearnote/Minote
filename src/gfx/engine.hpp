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
#include "gfx/modules/meshes.hpp"
#include "gfx/modules/world.hpp"
#include "gfx/modules/sky.hpp"
#include "gfx/modules/ibl.hpp"
#include "gfx/modules/bvh.hpp"
#include "gfx/objects.hpp"
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
	
	std::optional<modules::Meshes> meshes;
	Objects objects;
	modules::World world;
	Camera camera;
	std::optional<modules::Bvh> bvh;
	
	std::optional<modules::Atmosphere> atmosphere;
	std::optional<modules::IBLMap> ibl;
	
	auto createSwapchain(VkSwapchainKHR old = VK_NULL_HANDLE) -> vuk::Swapchain;
	void refreshSwapchain();
	
};

}

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
#include "gfx/objects.hpp"
#include "gfx/meshes.hpp"
#include "gfx/camera.hpp"
#include "gfx/world.hpp"
#if IMGUI
#include "gfx/imgui.hpp"
#endif //IMGUI
#include "gfx/sky.hpp"
#include "gfx/ibl.hpp"

namespace minote::gfx {

using namespace base;

struct Engine {
	
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
	Objects objects;
	World world;
	Camera camera;
	
	std::optional<Atmosphere> atmosphere;
	std::optional<IBLMap> ibl;
	
	auto createSwapchain(VkSwapchainKHR old = VK_NULL_HANDLE) -> vuk::Swapchain;
	void refreshSwapchain();
	
};

}

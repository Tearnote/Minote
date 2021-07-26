#pragma once

#include <optional>
#include "VkBootstrap.h"
#include "volk.h"
#include "vuk/Context.hpp"
#include "base/math.hpp"
#include "sys/window.hpp"
#include "gfx/module/sky.hpp"
#include "gfx/module/ibl.hpp"
#include "gfx/objects.hpp"
#include "gfx/meshes.hpp"
#include "gfx/camera.hpp"
#include "gfx/world.hpp"
#include "gfx/imgui.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;

// Graphics engine. Feed with meshes and objects, enjoy pretty pictures.
struct Engine {
	
	static constexpr auto VerticalFov = 45_deg;
	static constexpr auto NearPlane = 0.1_m;
	
	// Initialize Vulkan on the window.
	explicit Engine(sys::Window& window, Version version);
	~Engine();
	
	// After adding all meshes, call this to finalize them and upload to GPU.
	void uploadAssets();
	
	// Render all objects to the screen.
	void render();
	
	Engine(Engine const&) = delete;
	auto operator=(Engine const&) -> Engine& = delete;
	
	// Subcomponent access
	
	// Use to add models, then call uploadAssets()
	auto meshes() -> Meshes& { return *m_meshes; }
	
	// Use freely to add/remove/modify objects for drawing
	auto objects() -> Objects& { return *m_objects; }
	
	// Use freely to modify the rendering camera
	auto camera() -> Camera& { return m_camera; }
	
	
private:
	
	vkb::Instance m_instance;
	VkSurfaceKHR m_surface;
	vkb::Device m_device;
	vuk::SwapChainRef m_swapchain;
	std::optional<vuk::Context> m_context;
	ImguiData m_imguiData;
	
	std::optional<Meshes> m_meshes;
	std::optional<Objects> m_objects;
	Camera m_camera;
	
	World m_world;
	std::optional<Atmosphere> m_atmosphere;
	std::optional<IBLMap> m_ibl;
	
	auto createSwapchain(VkSwapchainKHR old = VK_NULL_HANDLE) -> vuk::Swapchain;
	void refreshSwapchain();
	
};

}

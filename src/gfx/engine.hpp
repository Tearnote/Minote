#pragma once

#include <optional>
#include "base/math.hpp"
#include "sys/vulkan.hpp"
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
	
	// Initialize the engine on a Vulkan-initialized window. Mesh list will
	// be consumed and uploaded to GPU.
	Engine(sys::Vulkan&, MeshList&&);
	~Engine();
	
	// Render all objects to the screen.
	void render();
	
	Engine(Engine const&) = delete;
	auto operator=(Engine const&) -> Engine& = delete;
	
	// Subcomponent access
	
	// Use freely to add/remove/modify objects for drawing
	auto objects() -> Objects& { return *m_objects; }
	
	// Use freely to modify the rendering camera
	auto camera() -> Camera& { return m_camera; }
	
private:
	
	sys::Vulkan& m_vk;
	
	ImguiData m_imguiData;
	std::optional<MeshBuffer> m_meshes;
	std::optional<Objects> m_objects;
	Camera m_camera;
	
	World m_world;
	std::optional<Atmosphere> m_atmosphere;
	std::optional<IBLMap> m_ibl;
	
	// Once a swapchain is detected to be invalid or out of date, use this function
	// to replace it with a fresh one.
	void refreshSwapchain();
	
};

}

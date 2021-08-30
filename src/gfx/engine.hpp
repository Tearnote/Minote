#pragma once

#include <optional>
#include <mutex>
#include "Tracy.hpp"
#include "base/math.hpp"
#include "sys/vulkan.hpp"
#include "gfx/resources/resourcepool.hpp"
#include "gfx/resources/cubemap.hpp"
#include "gfx/modules/sky.hpp"
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
	
	// Render all objects to the screen. If repaint is false, the function will
	// only render it no other thread is currently rendering. Otherwise it will
	// block until a frame can be successfully renderered. To avoid deadlock,
	// only ever use repaint=true on one thread.
	void render(bool repaint);
	
	// Use this function when the surface is resized to recreate the swapchain.
	void refreshSwapchain(uvec2 newSize);
	
	// Subcomponent access
	
	// Use freely to add/remove/modify objects for drawing
	auto objects() -> ObjectPool& { return *m_objects; }
	
	// Use freely to modify the rendering camera
	auto camera() -> Camera& { return m_camera; }
	
	// Not copyable, not movable
	Engine(Engine const&) = delete;
	auto operator=(Engine const&) -> Engine& = delete;
	Engine(Engine&&) = delete;
	auto operator=(Engine&&) -> Engine& = delete;
	
private:
	
	sys::Vulkan& m_vk;
	
	TracyLockable(std::mutex, m_renderLock);
	bool m_swapchainDirty;
	
	ImguiData m_imguiData;
	std::optional<MeshBuffer> m_meshes;
	std::optional<ObjectPool> m_objects;
	Camera m_camera;
	
	ResourcePool m_permPool;
	World m_world;
	Atmosphere m_atmosphere;
	
};

}

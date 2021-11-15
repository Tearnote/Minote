#pragma once

#include <optional>
#include <mutex>
#include "Tracy.hpp"
#include "base/math.hpp"
#include "base/time.hpp"
#include "sys/vulkan.hpp"
#include "gfx/resources/cubemap.hpp"
#include "gfx/resources/pool.hpp"
#include "gfx/effects/sky.hpp"
#include "gfx/materials.hpp"
#include "gfx/objects.hpp"
#include "gfx/models.hpp"
#include "gfx/camera.hpp"
#include "gfx/world.hpp"
#include "gfx/imgui.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;

// Graphics engine. Feed with models and objects, enjoy pretty pictures.
struct Engine {
	
	static constexpr auto VerticalFov = 50_deg;
	static constexpr auto NearPlane = 0.1_m;
	
	// Create the engine in uninitialized state.
	Engine(sys::Vulkan& vk): m_vk(vk),
		m_framerate(0.0f), m_lastFramerateCheck(0), m_framesSinceLastCheck(0) {}
	~Engine();
	
	void init(ModelList&&, MaterialList&&);
	
	// Render all objects to the screen. If repaint is false, the function will
	// only render it no other thread is currently rendering. Otherwise it will
	// block until a frame can be successfully renderered. To avoid deadlock,
	// only ever use repaint=true on one thread.
	void render();
	
	// Use this function when the surface is resized to recreate the swapchain.
	void refreshSwapchain(uvec2 newSize);
	
	// Subcomponent access
	
	// Use freely to add/remove/modify objects for drawing
	auto objects() -> ObjectPool& { return m_objects; }
	
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
	bool m_flushTemporalResources;
	
	f32 m_framerate;
	nsec m_lastFramerateCheck;
	u32 m_framesSinceLastCheck;
	
	ImguiData m_imguiData;
	ModelBuffer m_models;
	MaterialBuffer m_materials;
	ObjectPool m_objects;
	Camera m_camera;
	
	Pool m_permPool;
	Pool m_framePool;
	Pool m_swapchainPool;
	World m_world;
	Atmosphere m_atmosphere;
	
};

}

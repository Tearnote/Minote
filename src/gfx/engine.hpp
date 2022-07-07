#pragma once

#include <optional>
#include <mutex>
#include "vuk/resources/DeviceFrameResource.hpp"
#include "util/math.hpp"
#include "util/time.hpp"
#include "sys/vulkan.hpp"
#include "gfx/objects.hpp"
#include "gfx/models.hpp"
#include "gfx/camera.hpp"
#include "gfx/world.hpp"
#include "gfx/imgui.hpp"

namespace minote::gfx {

// Graphics engine. Feed with models and objects, enjoy pretty pictures.
struct Engine {
	
	static constexpr auto InflightFrames = 3u;
	static constexpr auto VerticalFov = 50_deg;
	static constexpr auto NearPlane = 0.1_m;
	
	// Create the engine in uninitialized state.
	Engine(sys::Vulkan& vk):
		m_vk(vk),
		m_deviceResource(*vk.context, InflightFrames),
		m_framerate(60.0f),
		m_lastFramerateCheck(0),
		m_framesSinceLastCheck(0),
		m_globalAllocator(m_deviceResource) {}
	~Engine();
	
	void init(ModelList&&);
	
	void render();
	
	// Use this function when the surface is resized to recreate the swapchain.
	void refreshSwapchain(uvec2 newSize);
	
	// Subcomponent access
	
	// Use freely to add/remove/modify objects for drawing
	auto objects() -> ObjectPool& { return m_objects; }
	
	// Use freely to modify the rendering camera
	auto camera() -> Camera& { return m_camera; }
	
	auto fps() const -> f32 { return m_framerate; }
	
	// Not copyable, not movable
	Engine(Engine const&) = delete;
	auto operator=(Engine const&) -> Engine& = delete;
	Engine(Engine&&) = delete;
	auto operator=(Engine&&) -> Engine& = delete;
	
private:
	
	sys::Vulkan& m_vk;
	
	vuk::DeviceSuperFrameResource m_deviceResource;
	vuk::Allocator m_globalAllocator;
	
	std::mutex m_renderLock;
	bool m_swapchainDirty;
	bool m_flushTemporalResources;
	
	f32 m_framerate;
	nsec m_lastFramerateCheck;
	u32 m_framesSinceLastCheck;
	
	ImguiData m_imguiData;
	ModelBuffer m_models;
	ObjectPool m_objects;
	World m_world;
	Camera m_camera;
	
	void renderFrame();
	
	friend struct Frame;
	
};

}

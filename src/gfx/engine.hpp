#pragma once

#include <optional>
#include <mutex>
#include "vuk/resources/DeviceFrameResource.hpp"
#include "util/service.hpp"
#include "util/math.hpp"
#include "util/time.hpp"
#include "sys/vulkan.hpp"
#include "gfx/objects.hpp"
#include "gfx/models.hpp"
#include "gfx/camera.hpp"
#include "gfx/world.hpp"
#include "gfx/imgui.hpp"

namespace minote {

// Graphics engine. Feed with models and objects, enjoy pretty pictures
struct Engine {
	
	static constexpr auto InflightFrames = 3u;
	static constexpr auto VerticalFov = 50_deg;
	static constexpr auto NearPlane = 0.1_m;
	
	Engine();
	~Engine();
	
	void init(ModelList&&);
	
	void render();
	
	// Use this function when the surface is resized to recreate the swapchain.
	void refreshSwapchain(uvec2 newSize);
	
	// Subcomponent access
	
	// Return the Imgui instance for debug input handling
	auto imgui() -> Imgui& { return m_imgui; }
	
	// Return the ObjectPool to add/remove/modify objects for drawing
	auto objects() -> ObjectPool& { return m_objects; }
	
	// Return the Camera to modify the viewport
	auto camera() -> Camera& { return m_camera; }
	
	auto fps() const -> f32 { return m_framerate; }
	
	Engine(Engine const&) = delete;
	auto operator=(Engine const&) -> Engine& = delete;
	
private:
	
	vuk::DeviceSuperFrameResource m_deviceResource;
	vuk::Allocator m_globalAllocator;
	
	std::mutex m_renderLock;
	bool m_swapchainDirty;
	bool m_flushTemporalResources;
	
	f32 m_framerate;
	nsec m_lastFramerateCheck;
	u32 m_framesSinceLastCheck;
	
	Imgui m_imgui;
	ModelBuffer m_models;
	ObjectPool m_objects;
	World m_world;
	Camera m_camera;
	
	void renderFrame();
	
	friend struct Frame;
	
};

inline Service<Engine> s_engine;

}

#pragma once

#include <optional>
#include <mutex>
#include "vuk/resources/DeviceFrameResource.hpp"
#include "util/service.hpp"
#include "util/math.hpp"
#include "util/time.hpp"
#include "sys/vulkan.hpp"
#include "gfx/effects/sky.hpp"
#include "gfx/objects.hpp"
#include "gfx/models.hpp"
#include "gfx/camera.hpp"
#include "gfx/world.hpp"
#include "gfx/imgui.hpp"

namespace minote {

// Feed with models and objects, enjoy pretty pictures
struct Renderer {
	
	static constexpr auto InflightFrames = 3u;
	static constexpr auto VerticalFov = 50_deg;
	static constexpr auto NearPlane = 0.1_m;
	static constexpr auto FramerateUpdate = 1_s;
	
	Renderer();
	~Renderer();
	
	void render();
	
	// Use this function when the surface is resized to recreate the swapchain
	void refreshSwapchain(uvec2 newSize);
	
	// Subcomponent access
	
	// Return Imgui to provide it with user inputs
	auto imgui() -> Imgui& { return m_imgui; }
	
	// Return ObjectPool to add/remove/modify objects for drawing
	auto objects() -> ObjectPool& { return m_objects; }
	
	// Return the Camera to modify the viewport
	auto camera() -> Camera& { return m_camera; }
	
	auto fps() const -> f32 { return m_framerate; }
	
	// Return allocators, for effects that need permanent or temporary internal allocations
	auto globalAllocator() -> vuk::Allocator& { return m_globalAllocator; }
	auto frameAllocator() -> vuk::Allocator& { return m_frameAllocator; }
	
	Renderer(Renderer const&) = delete;
	auto operator=(Renderer const&) -> Renderer& = delete;
	
private:
	
	vuk::DeviceSuperFrameResource m_deviceResource;
	vuk::Allocator m_globalAllocator;
	vuk::Allocator m_frameAllocator;
	
	// Thread-safety for situations like window resize
	std::mutex m_renderLock;
	// Swapchain is out of date, rendering is skipped and refreshSwapchain() must be called
	bool m_swapchainDirty;
	// All temporal resources were invalidated by a cut or swapchain resize
	bool m_flushTemporalResources;
	
	f32 m_framerate;
	nsec m_lastFramerateCheck;
	u32 m_framesSinceLastCheck;
	
	Imgui m_imgui;
	ModelBuffer m_models;
	ObjectPool m_objects;
	World m_world;
	Camera m_camera;
	
	optional<Atmosphere> m_atmosphere;
	
	void renderFrame(); // Common code of render() and refreshSwapchain()
	void calcFramerate();
	
};

inline Service<Renderer> s_renderer;

}

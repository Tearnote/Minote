#pragma once

#include <memory>
#include <mutex>
#include "vuk/resources/DeviceFrameResource.hpp"
#include "util/service.hpp"
#include "util/math.hpp"
#include "util/time.hpp"
#include "sys/vulkan.hpp"
#include "gfx/objects.hpp"
#include "gfx/models.hpp"
#include "gfx/camera.hpp"
#include "gfx/imgui.hpp"

namespace minote {

// Feed with models and objects, enjoy pretty pictures
struct Renderer {
	
	// Global details of the game world
	struct World {
		vec3 sunDirection;
		vec3 sunIlluminance;
	};
	
	static constexpr auto InflightFrames = 3u;
	static constexpr auto FramerateUpdate = 1_s;
	
	Renderer();
	~Renderer();
	
	// Draw the world and present to display
	void render();
	
	// Use this function when the surface is resized to recreate the swapchain
	// and re-enable normal drawing
	void refreshSwapchain(uvec2 newSize);
	
	// Subcomponent access
	
	// Return Imgui to provide it with user inputs
	auto imgui() -> Imgui& { return m_imgui; }
	
	// Return ObjectPool to add/remove/modify objects for drawing
	auto objects() -> ObjectPool& { return m_objects; }
	
	// Return the world properties to adjust global details like time of day
	auto world() -> World& { return m_world; }
	
	// Return the Camera to modify how the world is viewed
	auto camera() -> Camera& { return m_camera; }
	
	// Current framerate, updated once a second
	auto fps() const -> f32 { return m_framerate; }
	
	// Allocators for effects that need permanent or temporary internal allocations
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
	
	f32 m_framerate;
	nsec m_lastFramerateCheck;
	u32 m_framesSinceLastCheck;
	
	Imgui m_imgui;
	ModelBuffer m_models;
	ObjectPool m_objects;
	World m_world;
	Camera m_camera;
	
	void renderFrame(); // Common code of render() and refreshSwapchain()
	void beginFrame();
	void calcFramerate();
	auto buildRenderGraph() -> std::shared_ptr<vuk::RenderGraph>;
	void submitAndPresent(std::shared_ptr<vuk::RenderGraph>);
	
	struct Impl;
	std::unique_ptr<Impl> m_impl;
	
};

using World = Renderer::World;

inline Service<Renderer> s_renderer;

}

#pragma once

#include <optional>
#include <memory>
#include <mutex>
#include "vuk/resources/DeviceFrameResource.hpp"
#include "util/service.hpp"
#include "math.hpp"
#include "util/time.hpp"
#include "sys/vulkan.hpp"
#include "gfx/objects.hpp"
#include "gfx/models.hpp"
#include "gfx/camera.hpp"
#include "gfx/imgui.hpp"

namespace minote {

// Feed with models and objects, enjoy pretty pictures
struct Renderer {
	
	static constexpr auto InflightFrames = 3u;
	static constexpr auto FramerateUpdate = 1_s;
	
	Renderer();
	~Renderer();
	
	// Push models to GPU so that they're ready to draw
	void uploadModels(ModelList&&);
	
	// Draw the world and present to display
	void render();
	
	// Use this function when the surface is resized to recreate the swapchain
	// and re-enable normal drawing
	void refreshSwapchain(uint2 newSize);
	
	// Subcomponent access
	
	// Return Imgui to provide it with user inputs
	auto imgui() -> Imgui& { return m_imgui; }
	
	// Return ObjectPool to add/remove/modify objects for drawing
	auto objects() -> ObjectPool& { return m_objects; }
	
	// Return the Camera to modify how the world is viewed
	auto camera() -> Camera& { return m_camera; }
	
	// Current framerate, updated once a second
	auto fps() const -> float { return m_framerate; }
	
	// Allocator for effects that need temporary internal allocations
	auto frameAllocator() -> vuk::Allocator& { return *m_frameAllocator; }
	
	Renderer(Renderer const&) = delete;
	auto operator=(Renderer const&) -> Renderer& = delete;
	
private:
	
	vuk::DeviceSuperFrameResource m_deviceResource;
	vuk::Allocator m_multiFrameAllocator;
	vuk::DeviceFrameResource* m_frameResource;
	std::optional<vuk::Allocator> m_frameAllocator;
	
	// Thread-safety for situations like window resize
	std::mutex m_renderLock;
	// Swapchain is out of date, rendering is skipped and refreshSwapchain() must be called
	bool m_swapchainDirty;
	
	float m_framerate;
	nsec m_lastFramerateCheck;
	uint m_framesSinceLastCheck;
	
	Imgui m_imgui;
	ModelBuffer m_models;
	ObjectPool m_objects;
	Camera m_camera;
	
	void renderFrame(); // Common code of render() and refreshSwapchain()
	void beginFrame();
	void calcFramerate();
	void executeRenderGraph();
	void endFrame();
	
	struct Impl;
	std::unique_ptr<Impl> m_impl;
	
};

inline util::Service<Renderer> s_renderer;

}

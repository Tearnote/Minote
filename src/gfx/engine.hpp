#pragma once

#include "config.hpp"

#include <optional>
#include <span>
#include "VkBootstrap.h"
#include "volk.h"
#include "vuk/Context.hpp"
#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "base/version.hpp"
#include "base/hashmap.hpp"
#include "base/id.hpp"
#include "sys/window.hpp"
#include "gfx/instances.hpp"
#include "gfx/meshes.hpp"
#if IMGUI
#include "gfx/imgui.hpp"
#endif //IMGUI
#include "gfx/sky.hpp"

namespace minote::gfx {

using namespace base;

struct Engine {

	using Instance = Instances::Instance;

	explicit Engine(sys::Window& window, Version version);
	~Engine();

	void addModel(std::string_view name, std::span<char const> model);
	void uploadAssets();

	void setCamera(glm::vec3 eye, glm::vec3 center, glm::vec3 up = {0.0f, 1.0f, 0.0f});
	void enqueue(ID mesh, std::span<Instance const> instances);

	void render();

	Engine(Engine const&) = delete;
	auto operator=(Engine const&) -> Engine& = delete;

private:

	vkb::Instance instance;
	VkSurfaceKHR surface;
	vkb::Device device;
	vuk::SwapChainRef swapchain;
	std::optional<vuk::Context> context;
#if IMGUI
	ImguiData imguiData;
#endif //IMGUI

	Meshes meshes;
	Instances instances;

	vuk::Unique<vuk::Buffer> verticesBuf;
	vuk::Unique<vuk::Buffer> normalsBuf;
	vuk::Unique<vuk::Buffer> colorsBuf;
	vuk::Unique<vuk::Buffer> indicesBuf;

	struct World {
		glm::mat4 view;
		glm::mat4 projection;
		glm::mat4 viewProjection;
	} world;

	struct Camera {
		glm::vec3 eye;
		glm::vec3 center;
		glm::vec3 up;
	} camera;

	std::optional<vuk::Texture> cubemap;
	std::vector<vuk::Unique<vuk::ImageView>> cubemapMips;
	vuk::Unique<vuk::PersistentDescriptorSet> cubemapPds;

	std::optional<Sky> sky;

	auto createSwapchain(VkSwapchainKHR old = VK_NULL_HANDLE) -> vuk::Swapchain;
	void refreshSwapchain();

};

}

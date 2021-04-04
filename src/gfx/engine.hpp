#pragma once

#include "config.hpp"

#include <optional>
#include <span>
#include "VkBootstrap.h"
#include "volk.h"
#include "vuk/Context.hpp"
#include "base/version.hpp"
#include "base/hashmap.hpp"
#include "base/id.hpp"
#include "sys/window.hpp"
#if IMGUI
#include "gfx/imgui.hpp"
#endif //IMGUI
#include "gfx/world.hpp"

namespace minote::gfx {

using namespace base;

struct Engine {

	struct Instance {

		glm::mat4 transform = glm::mat4(1.0f);
		glm::vec4 tint = {1.0f, 1.0f, 1.0f, 1.0f};

		f32 ambient;
		f32 diffuse;
		f32 specular;
		f32 shine;

	};

	explicit Engine(sys::Window& window, Version version);
	~Engine();

	void setup();
	void render();

	void setBackground(glm::vec3 color);
	void setLightSource(glm::vec3 position, glm::vec3 color);
	void setCamera(glm::vec3 eye, glm::vec3 center, glm::vec3 up = {0.0f, 1.0f, 0.0f});

	void enqueue(ID mesh, std::span<Instance const> instances);

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

	hashmap<ID, vuk::Unique<vuk::Buffer>> meshes;
	hashmap<ID, std::vector<Instance>> instances;

	World world;
	struct Camera {
		glm::vec3 eye;
		glm::vec3 center;
		glm::vec3 up;
	} camera;

	std::optional<vuk::Texture> env;

	auto createSwapchain(VkSwapchainKHR old = VK_NULL_HANDLE) -> vuk::Swapchain;
	void refreshSwapchain();

};

}

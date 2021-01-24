#pragma once

#include <string_view>
#include <span>
#include <glm/vec3.hpp>
#include "base/svector.hpp"
#include "base/version.hpp"
#include "base/types.hpp"
#include "base/id.hpp"
#include "sys/window.hpp"
#include "sys/glfw.hpp"
#include "gfx/swapchain.hpp"
#include "gfx/technique.hpp"
#include "gfx/material.hpp"
#include "gfx/samplers.hpp"
#include "gfx/commands.hpp"
#include "gfx/context.hpp"
#include "gfx/targets.hpp"
#include "gfx/present.hpp"
#include "gfx/bloom.hpp"
#include "gfx/world.hpp"
#include "gfx/base.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;

struct Engine {

	Engine(sys::Glfw&, sys::Window&, std::string_view name, Version appVersion);
	~Engine();

	void setBackground(glm::vec3 color);
	void setLightSource(glm::vec3 position, glm::vec3 color);
	void setCamera(glm::vec3 eye, glm::vec3 center, glm::vec3 up = {0.0f, 1.0f, 0.0f});

	void enqueueDraw(ID mesh, ID technique, std::span<Instance const> instances,
		Material material, MaterialData const& materialData = {});

	void render();

private:

	struct Camera {

		glm::vec3 eye;
		glm::vec3 center;
		glm::vec3 up;

	};

	struct DelayedOp {

		u64 deadline;
		std::function<void()> func;

	};

	u64 frameCounter = 0;
	svector<DelayedOp, 64> delayedOps;

	Context ctx;
	Swapchain swapchain;
	Commands commands;

	Samplers samplers;
	MeshBuffer meshes;
	World world;
	Camera camera;
	Targets targets;

	TechniqueSet techniques;
	Bloom bloom;
	Present present;

	void refresh();

};

}

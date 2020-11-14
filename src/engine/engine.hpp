/**
 * A struct bringing together all basic utilities
 * @file
 */

#pragma once

#include "sys/window.hpp"
#include "engine/mapper.hpp"
#include "engine/scene.hpp"
#include "engine/frame.hpp"
#include "store/shaders.hpp"
#include "store/models.hpp"

namespace minote {

struct Engine {

	Window& window;
	Mapper& mapper;
	Frame& frame;
	Scene& scene;

	Shaders& shaders;
	Models& models;

};

}

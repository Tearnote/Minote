// Minote - engine/engine.hpp
// A struct bringing together all basic utilities

#pragma once

#include "sys/window.hpp"
#include "engine/mapper.hpp"
#include "engine/scene.hpp"
#include "engine/frame.hpp"
#include "store/shaders.hpp"
#include "store/models.hpp"
#include "store/fonts.hpp"

namespace minote {

// Must be created using a designated initializer
struct Engine {

	// *** Global facilities ***

	Window& window;
	Mapper& mapper;
	Frame& frame;
	Scene& scene;

	// *** Content stores ***

	Shaders& shaders;
	Models& models;
	Fonts& fonts;

};

}

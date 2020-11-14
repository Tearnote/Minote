/**
 * Storage for all available models
 * @file
 */

#pragma once

#include "engine/model.hpp"
#include "store/shaders.hpp"

namespace minote {

struct Models {

	ModelFlat sync;
	ModelPhong block;
	ModelFlat scene;
	ModelFlat guide;
	ModelFlat border;
	ModelFlat particle;

	void create(Shaders& shaders);

	void destroy();

};

}

// Minote - store/models.hpp
// Storage for all available models

#pragma once

#include "base/hashmap.hpp"
#include "base/string.hpp"
#include "engine/model.hpp"
#include "store/shaders.hpp"

namespace minote {

struct Models {

	// Basic one triangle model used to defeat GPUs' frame caching
	ModelFlat sync;

	// Traditional block shape
	ModelPhong block;

	// Decoration around the field to show where the borders are
	ModelFlat field;

	// Optional column guide to make vertical aiming easier
	ModelFlat guide;

	// The semi-transparent border around the shape of the stack
	ModelFlat border;

	// A small particle piece to draw in great quantities
	ModelFlat particle;

	// Create all the models, uploading the vertex data to the GPU. After this call, they can
	// be freely accessed and used for drawing.
	explicit Models(Shaders& shaders) noexcept;

	// Clean up all game models.
	~Models() noexcept;

};

}

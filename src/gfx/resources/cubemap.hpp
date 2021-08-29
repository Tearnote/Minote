#pragma once

#include "vuk/Context.hpp"
#include "vuk/Image.hpp"
#include "base/containers/vector.hpp"
#include "base/types.hpp"
#include "gfx/resources/texture2d.hpp"

namespace minote::gfx {

using namespace base;

// Array of 6 textures, functioning as a sampler that has a value for every
// angle vector. Mipmaps are created, and each mipmap level can be accessed
// with a corresponding view.
struct Cubemap: public Texture2D {
	
	// nth entry is an image array view into nth mipmap level
	svector<vuk::Unique<vuk::ImageView>, 16> arrayViews;
	
	// Construct an invalid cubemap. It will destruct safely, but the only
	// meaningful use is to overwrite it later.
	Cubemap() = default;
	
	// Create the cubemap. Each face will be a square.
	Cubemap(vuk::PerThreadContext&, vuk::Name, u32 size, vuk::Format, vuk::ImageUsageFlags);
	
};

}

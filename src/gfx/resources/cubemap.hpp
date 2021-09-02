#pragma once

#include "vuk/Image.hpp"
#include "base/types.hpp"
#include "gfx/resources/texture2d.hpp"

namespace minote::gfx {

using namespace base;

// Array of 6 textures, functioning as a sampler that has a value for every
// angle vector. Mipmaps are created, and each mipmap level can be accessed
// with a corresponding view.
struct Cubemap: public Texture2D {
	
	// Create a cubemap image view into a specified mipmap level.
	auto mipView(u32 mip) -> vuk::Unique<vuk::ImageView>;
	
	// Create a 6-layer image view into a specified mipmap level.
	auto mipArrayView(u32 mip) -> vuk::Unique<vuk::ImageView>;
	
};

}

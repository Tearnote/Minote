#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "vuk/Image.hpp"
#include "base/containers/array.hpp"
#include "base/types.hpp"
#include "base/math.hpp"

namespace minote::gfx {

using namespace base;

// Array of 6 textures, functioning as a sampler that has a value for every
// angle vector. Mipmaps are generated, and each mipmap level can be accessed
// with a corresponding view.
struct Cubemap {
	
	vuk::Name name;
	vuk::Texture texture;
	// nth entry is an image array view into nth mipmap level
	array<vuk::Unique<vuk::ImageView>> arrayViews;
	
	// Create the cubemap. Each face will be a square.
	Cubemap(vuk::PerThreadContext&, vuk::Name, u32 size, vuk::Format, vuk::ImageUsageFlags);
	
	// Return the size of a cubemap face.
	auto size() const -> uvec2 { return uvec2{texture.extent.width, texture.extent.height}; }
	
	// Return the surface format.
	auto format() const -> vuk::Format { return texture.format; }
	
	// Attach cubemap to rendergraph
	void attach(vuk::RenderGraph&, vuk::Access initial, vuk::Access final);
	
};

}

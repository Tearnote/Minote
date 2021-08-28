#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "vuk/Image.hpp"
#include "base/containers/vector.hpp"
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
	svector<vuk::Unique<vuk::ImageView>, 16> arrayViews;
	
	// Construct an invalid cubemap. It will destruct safely, but the only
	// meaningful use is to overwrite it later.
	Cubemap() = default;
	
	// Create the cubemap. Each face will be a square.
	Cubemap(vuk::PerThreadContext&, vuk::Name, u32 size, vuk::Format, vuk::ImageUsageFlags);
	
	// Return the size of a cubemap face.
	auto size() const -> uvec2 { return uvec2{texture.extent.width, texture.extent.height}; }
	
	// Return the surface format.
	auto format() const -> vuk::Format { return texture.format; }
	
	// Declare as a vuk::Resource.
	auto resource(vuk::Access) const -> vuk::Resource;
	
	// Attach cubemap to rendergraph
	void attach(vuk::RenderGraph&, vuk::Access initial, vuk::Access final);
	
};

}

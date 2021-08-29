#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "vuk/Image.hpp"
#include "base/types.hpp"
#include "base/math.hpp"

namespace minote::gfx {

using namespace base;

// Array of 6 textures, functioning as a sampler that has a value for every
// angle vector. Mipmaps are generated, and each mipmap level can be accessed
// with a corresponding view.
struct Texture2D {
	
	vuk::Name name;
	vuk::Texture texture;
	
	// Construct an invalid texture. It will destruct safely, but the only
	// meaningful use is to overwrite it later.
	Texture2D() = default;
	
	// Create the texture.
	Texture2D(vuk::PerThreadContext&, vuk::Name, uvec2 size, vuk::Format,
		vuk::ImageUsageFlags, u32 mips = 1);
	
	// Create an image view into a specified mipmap level.
	auto mipView(u32 mip) -> vuk::Unique<vuk::ImageView>;
	
	// Return the size of the texture.
	auto size() const -> uvec2 { return uvec2{texture.extent.width, texture.extent.height}; }
	
	// Return the surface format.
	auto format() const -> vuk::Format { return texture.format; }
	
	// Declare as a vuk::Resource.
	auto resource(vuk::Access) const -> vuk::Resource;
	
	// Attach texture to rendergraph
	void attach(vuk::RenderGraph&, vuk::Access initial, vuk::Access final);
	
};

}

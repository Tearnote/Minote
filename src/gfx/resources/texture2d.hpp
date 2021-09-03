#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Image.hpp"
#include "base/types.hpp"
#include "base/math.hpp"

namespace minote::gfx {

using namespace base;

struct Texture2D {
	
	vuk::Name name;
	vuk::Texture* handle = nullptr;
	
	// Create an image view into a specified mipmap level.
	auto mipView(u32 mip) -> vuk::Unique<vuk::ImageView>;
	
	// Return the size of the texture.
	auto size() const -> uvec2 { return uvec2{handle->extent.width, handle->extent.height}; }
	
	// Return the surface format.
	auto format() const -> vuk::Format { return handle->format; }
	
	// Declare as a vuk::Resource.
	auto resource(vuk::Access) const -> vuk::Resource;
	
	// Attach texture to rendergraph
	void attach(vuk::RenderGraph&, vuk::Access initial, vuk::Access final) const;
	
	// Convertible to vuk::ImageView
	operator vuk::ImageView() const { return *handle->view; }
	
};

}

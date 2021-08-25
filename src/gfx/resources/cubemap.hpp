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
	
	vuk::Image image;
	vuk::ImageView view;
	// nth entry is an image array view into nth mipmap level
	svector<vuk::ImageView, 16> arrayViews;
	
	// Construct an invalid cubemap. It will destruct safely, but the only
	// meaningful use is to overwrite it later.
	Cubemap() = default;
	
	// Create the cubemap. Each face will be a square.
	Cubemap(vuk::PerThreadContext&, vuk::Name, u32 size, vuk::Format, vuk::ImageUsageFlags);
	
	// Destroy the cubemap after the current frame is fully finished drawing.
	// If the cubemap is valid, this must be called.
	void recycle(vuk::PerThreadContext&);
	
	// Return the size of a cubemap face.
	auto size() const -> uvec2 { return m_size; }
	
	// Return the surface format.
	auto format() const -> vuk::Format { return m_format; }
	
	// Attach cubemap to rendergraph
	void attach(vuk::RenderGraph&, vuk::Access initial, vuk::Access final);
	
private:
	
	uvec2 m_size;
	vuk::Format m_format;
	
};

}

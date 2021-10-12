#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Image.hpp"
#include "vuk/Name.hpp"
#include "base/types.hpp"
#include "base/math.hpp"
#include "gfx/resources/pool.hpp"

namespace minote::gfx {

using namespace base;

struct Texture2DMS {
	
	vuk::Name name;
	vuk::Texture* handle = nullptr;
	
	// Construct a texture inside a pool. If the pool already contained a texture under the same
	// name, the existing one is retrieved instead.
	static auto make(Pool&, vuk::Name, uvec2 size, vuk::Format,
		vuk::ImageUsageFlags, vuk::SampleCountFlagBits) -> Texture2DMS;
	
	// Return the size of the texture.
	[[nodiscard]]
	auto size() const -> uvec2 { return uvec2{handle->extent.width, handle->extent.height}; }
	
	// Return the count of samples per pixel.
	[[nodiscard]]
	auto samples() const -> u32 { return u32(handle->sample_count.count); }
	
	// Return the surface format.
	[[nodiscard]]
	auto format() const -> vuk::Format { return handle->format; }
	
	// Declare as a vuk::Resource.
	[[nodiscard]]
	auto resource(vuk::Access) const -> vuk::Resource;
	
	// Attach texture to rendergraph
	void attach(vuk::RenderGraph&, vuk::Access initial, vuk::Access final, vuk::Clear = {}) const;
	
	// Convertible to vuk::ImageView
	operator vuk::ImageView() const { return *handle->view; }
	
};

}

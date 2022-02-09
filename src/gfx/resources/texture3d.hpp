#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Image.hpp"
#include "vuk/Name.hpp"
#include "base/types.hpp"
#include "base/math.hpp"
#include "gfx/resources/pool.hpp"

namespace minote::gfx {

using namespace base;

struct Texture3D {
	
	vuk::Name name;
	vuk::Texture* handle = nullptr;
	
	// Construct a texture inside a pool. If the pool already contained a texture under the same
	// name, the existing one is retrieved instead.
	static auto make(Pool&, vuk::Name, uvec3 size, vuk::Format, vuk::ImageUsageFlags) -> Texture3D;
	
	// Return the size of the texture.
	[[nodiscard]]
	auto size() const -> uvec3 { return uvec3{handle->extent.width, handle->extent.height, handle->extent.depth}; }
	
	// Return the surface format.
	[[nodiscard]]
	auto format() const -> vuk::Format { return handle->format; }
	
	// Declare as a vuk::Resource.
	[[nodiscard]]
	auto resource(vuk::Access) const -> vuk::Resource;
	
	// Attach texture to rendergraph
	void attach(vuk::RenderGraph&, vuk::Access initial, vuk::Access final, vuk::Clear = {}) const;
	
	operator bool() const { return handle != nullptr; }
	
	// Convertible to vuk::ImageView
	operator vuk::ImageView() const { return *handle->view; }
	
};

}

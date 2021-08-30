#pragma once

#include <span>
#include "vuk/RenderGraph.hpp"
#include "vuk/Buffer.hpp"
#include "base/types.hpp"

namespace minote::gfx {

using namespace base;

template<typename T>
struct Buffer {
	
	vuk::Name name;
	vuk::Buffer* handle = nullptr;
	
	// Size of the buffer in bytes.
	auto size() const -> usize { return handle->size; }
	
	// Declare as a vuk::Resource.
	auto resource(vuk::Access) const -> vuk::Resource;
	
	// Attach the buffer to the rendergraph.
	void attach(vuk::RenderGraph&, vuk::Access initial, vuk::Access final);
	
	// Convertible to vuk::Buffer
	operator vuk::Buffer() const { return *handle; }

	
};

}

#include "gfx/resources/buffer.tpp"

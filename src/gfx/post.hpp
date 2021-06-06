#pragma once

#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"

namespace minote::gfx {

struct Post {
	
	Post(vuk::PerThreadContext&);
	
	auto tonemap(vuk::Name source, vuk::Name target,
		vuk::Extent2D targetSize) -> vuk::RenderGraph;
	
private:
	
	inline static bool pipelinesCreated = false;
	
};

}

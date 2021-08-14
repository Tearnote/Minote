#pragma once

#include "base/containers/array.hpp"
#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "vuk/Image.hpp"

namespace minote::gfx {

using namespace base;

// IBL is a cubemap that represents the environment for the purposes of lighting.
// The last mipmap is usable for diffuse, the rest are specular for varying
// values of roughness. After writing into all faces of mip 0 of Unfiltered_n,
// call filter() to generate all mips inside Filtered_n.
struct IBLMap {
	
	static constexpr auto Unfiltered_n = "ibl_map_unfiltered";
	static constexpr auto Filtered_n = "ibl_map_filtered";
	
	static constexpr auto BaseSize = 256u; // Current filtering method doesn't
	                                       // support any other size.
	static constexpr auto Format = vuk::Format::eR16G16B16A16Sfloat;
	static constexpr auto MipCount = 1u + 7u;
	
	// Create the IBL cubemaps. They are persistent resources.
	IBLMap(vuk::PerThreadContext&);
	
	// Using data from mip 0 of Unfiltered_n, generate all mips of Filtered_n.
	auto filter() -> vuk::RenderGraph;
	
private:
	
	inline static bool pipelinesCreated = false;
	
	vuk::Texture mapUnfiltered;
	vuk::Texture mapFiltered;
	sarray<vuk::Unique<vuk::ImageView>, MipCount> arrayViewsUnfiltered;
	sarray<vuk::Unique<vuk::ImageView>, MipCount> arrayViewsFiltered;
	
};

}

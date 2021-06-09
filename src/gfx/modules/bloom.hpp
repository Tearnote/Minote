#pragma once

#include <array>
#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"

namespace minote::gfx::modules {

struct Bloom {
	
	static constexpr auto Bloom_n = "bloom";
	
	static constexpr auto BloomFormat = vuk::Format::eB10G11R11UfloatPack32;
	static constexpr auto BloomStrength = 6u;
	
	vuk::Extent2D size;
	vuk::Texture bloom;
	std::array<vuk::Unique<vuk::ImageView>, BloomStrength> bloomViews;
	
	Bloom(vuk::PerThreadContext&, vuk::Extent2D size);
	
	auto apply(vuk::Name target) -> vuk::RenderGraph;
	
private:
	
	inline static bool pipelinesCreated = false;
	
};

}

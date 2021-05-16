#pragma once

#include <array>
#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"
#include "vuk/Image.hpp"

namespace minote::gfx {

struct IBLMap {

	constexpr static auto BaseSize = 256u;
	constexpr static auto Format = vuk::Format::eR16G16B16A16Sfloat;
	constexpr static auto MipCount = 1u + 7u;

	vuk::Texture mapUnfiltered;
	vuk::Texture mapFiltered;
	std::array<vuk::Unique<vuk::ImageView>, MipCount> arrayViewsUnfiltered;
	std::array<vuk::Unique<vuk::ImageView>, MipCount> arrayViewsFiltered;

	IBLMap(vuk::Context&, vuk::PerThreadContext&);

	auto filter() -> vuk::RenderGraph;

};

}

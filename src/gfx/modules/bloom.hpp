#pragma once

#include "base/containers/array.hpp"
#include "base/math.hpp"
#include "vuk/RenderGraph.hpp"
#include "vuk/Context.hpp"

namespace minote::gfx {

using namespace base;

// Bloom effect. Blends an image with a blurred version of itself.
// This implementation has no thresholding to better mimic naked-eye glare,
// and uses a low-pass filter to avoid fireflies that are commonly seen
// in HDR source images. Blur width is resolution-independent.
struct Bloom {
	
	static constexpr auto Bloom_n = "bloom";
	
	//TODO modify shaders to make the more compact format work
	// static constexpr auto BloomFormat = vuk::Format::eB10G11R11UfloatPack32;
	static constexpr auto BloomFormat = vuk::Format::eR16G16B16A16Sfloat;
	// More passes increases blur width at a small performance cost
	static constexpr auto BloomPasses = 6u;
	// Because the blending is additive, the strength multiplier needs to be very small
	static constexpr auto BloomStrength = 1.0f / 64.0f;
	
	// Initialize the effect. A Bloom instance can afterwards be used
	// with any image that has the specified dimensions and BloomFormat.
	Bloom(vuk::PerThreadContext&, uvec2 size);
	
	auto size() const -> uvec2 { return m_size; }
	
	// Create a pass that applies bloom to the specified image.
	auto apply(vuk::Name target) -> vuk::RenderGraph;
	
private:
	
	inline static bool pipelinesCreated = false;
	
	uvec2 m_size;
	// Mipmapped intermediate texture
	vuk::Texture m_bloom;
	// An image view for each mipmap level of m_bloom
	sarray<vuk::Unique<vuk::ImageView>, BloomPasses> m_bloomViews;
	
};

}

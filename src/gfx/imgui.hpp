#pragma once

#include <memory>
#include "vuk/SampledImage.hpp"
#include "vuk/RenderGraph.hpp"
#include "util/vector.hpp"

namespace minote {

// Imgui wrapper to simplify the invocations
struct Imgui {
	
	explicit Imgui(vuk::Allocator&);
	~Imgui();
	
	void render(vuk::Allocator&, vuk::RenderGraph&,
		vuk::Name targetFrom, vuk::Name targetTo,
		ivector<vuk::SampledImage> const& sampledImages);
	
	Imgui(Imgui const&) = delete;
	auto operator=(Imgui const&) -> Imgui& = delete;
	
private:
	
	static inline bool m_exists = false;
	vuk::Texture m_fontTex;
	std::unique_ptr<vuk::SampledImage> m_fontSI;
	
	void setTheme();
	void uploadFont(vuk::Allocator&); // Initialize m_fontTex, m_fontSI
	
};

}

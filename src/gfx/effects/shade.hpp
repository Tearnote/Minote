#pragma once

#include "gfx/effects/instanceList.hpp"
#include "gfx/effects/visibility.hpp"
#include "gfx/resource.hpp"
#include "util/math.hpp"

namespace minote {

struct Shade {
	
	// Flat shading
	static auto flat(Worklist&, ModelBuffer&, InstanceList&, Visibility&, TriangleList&,
		Texture2D<float4> _target) -> Texture2D<float4>;
	
	// Build required shaders; optional
	static void compile();
	
private:
	
	static inline bool m_compiled = false;
	
};

}

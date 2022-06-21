#pragma once

#include "vuk/Context.hpp"
#include "vuk/Name.hpp"
#include "base/types.hpp"
#include "base/math.hpp"

namespace minote::gfx {

using namespace base;

// Struct of global data, commonly used by shaders.
struct World {
	
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	mat4 viewProjectionInverse;
	mat4 prevViewProjection;
	uvec2 viewportSize;
	f32 nearPlane;
	f32 pad0;
	vec3 cameraPos;
	u32 frameCounter;
	
	vec3 sunDirection;
	f32 pad1;
	vec3 sunIlluminance;
	
	// Upload the world data to the GPU, to be used as a uniform.
	// auto upload(Pool&, vuk::Name) const -> Buffer<World>;
	
};

}

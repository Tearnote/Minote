#pragma once

#include "volk/volk.h"
#include "gfx/context.hpp"

namespace minote::gfx {

struct Samplers {

	VkSampler linear;

	void init(Context& ctx);
	void cleanup(Context& ctx);

};

}

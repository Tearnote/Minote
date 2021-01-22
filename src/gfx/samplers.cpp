#include "gfx/samplers.hpp"

#include "sys/vk/debug.hpp"

namespace minote::gfx {

namespace vk = sys::vk;

void Samplers::init(Context& ctx) {
	auto const samplerCI = VkSamplerCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	};
	VK(vkCreateSampler(ctx.device, &samplerCI, nullptr, &linear));
	vk::setDebugName(ctx.device, linear, "Samplers::linear");
}

void Samplers::cleanup(Context& ctx) {
	vkDestroySampler(ctx.device, linear, nullptr);
}

}

#pragma once

#include <array>
#include <span>
#include "volk/volk.h"
#include "base/types.hpp"
#include "sys/vk/base.hpp"

namespace minote::sys::vk {

struct Shader {

	VkShaderModule vert;
	VkShaderModule frag;
	std::array<VkDescriptorSetLayout, 4> descriptorSetLayouts;

};

auto createShader(VkDevice device,
	std::span<base::u32 const> vertSrc, std::span<base::u32 const> fragSrc,
	std::span<VkDescriptorSetLayoutCreateInfo const> layoutCIs) -> Shader;

void destroyShader(VkDevice device, Shader& shader);

}

#pragma once

#include <array>
#include <span>
#include "volk/volk.h"
#include "base/types.hpp"
#include "sys/vk/base.hpp"

namespace minote::sys::vk {

using namespace base;

struct Shader {

	VkShaderModule vert;
	VkShaderModule frag;

};

struct Descriptor {

	VkDescriptorType type;
	VkShaderStageFlags stages;
	VkSampler sampler;

};

auto createShader(VkDevice device, std::span<u32 const> vertSrc, std::span<u32 const> fragSrc) -> Shader;

void destroyShader(VkDevice device, Shader& shader);

auto createDescriptorSetLayout(VkDevice device, std::span<Descriptor const> descriptors) -> VkDescriptorSetLayout;

}

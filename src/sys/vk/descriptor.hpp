#pragma once

#include <span>
#include "volk/volk.h"

namespace minote::sys::vk {

struct Descriptor {

	VkDescriptorType type;
	VkShaderStageFlags stages;
	VkSampler sampler;

};

auto createDescriptorSetLayout(VkDevice device, std::span<Descriptor const> descriptors) -> VkDescriptorSetLayout;

auto allocateDescriptorSet(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout) -> VkDescriptorSet;

}

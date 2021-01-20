#pragma once

#include <span>
#include "volk/volk.h"
#include "base/types.hpp"
#include "sys/vk/buffer.hpp"
#include "sys/vk/image.hpp"

namespace minote::sys::vk {

using namespace base;

struct Descriptor {

	VkDescriptorType type;
	VkShaderStageFlags stages;
	VkSampler sampler;

};

auto createDescriptorSetLayout(VkDevice device, std::span<Descriptor const> descriptors) -> VkDescriptorSetLayout;

auto allocateDescriptorSet(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout) -> VkDescriptorSet;

auto makeDescriptorSetBufferWrite(VkDescriptorSet target, u32 descriptor, Buffer& buffer, VkDescriptorType type) -> VkWriteDescriptorSet;

auto makeDescriptorSetImageWrite(VkDescriptorSet target, u32 descriptor, Image& image, VkDescriptorType type, VkImageLayout layout) -> VkWriteDescriptorSet;

void updateDescriptorSets(VkDevice device, std::span<VkWriteDescriptorSet const> writes);

}

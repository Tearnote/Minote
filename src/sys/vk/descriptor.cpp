#include "sys/vk/descriptor.hpp"

#include <algorithm>
#include "sys/vk/base.hpp"
#include "base/types.hpp"

namespace minote::sys::vk {

using namespace base;
namespace ranges = std::ranges;

auto createDescriptorSetLayout(VkDevice device, std::span<Descriptor const> descriptors) -> VkDescriptorSetLayout {
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	bindings.reserve(descriptors.size());
	ranges::transform(descriptors, std::back_inserter(bindings), [&](auto& descriptor) {
		return VkDescriptorSetLayoutBinding{
			.binding = static_cast<u32>(bindings.size()),
			.descriptorType = descriptor.type,
			.descriptorCount = 1,
			.stageFlags = descriptor.stages,
			.pImmutableSamplers = &descriptor.sampler,
		};
	});

	auto const layoutCI = VkDescriptorSetLayoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = static_cast<u32>(bindings.size()),
		.pBindings = bindings.data(),
	};
	VkDescriptorSetLayout result;
	VK(vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &result));
	return result;
}

auto allocateDescriptorSet(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout) -> VkDescriptorSet {
	auto const descriptorSetAI = VkDescriptorSetAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &layout,
	};
	VkDescriptorSet result;
	VK(vkAllocateDescriptorSets(device, &descriptorSetAI, &result));
	return result;
}

}

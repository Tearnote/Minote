#include "sys/vk/shader.hpp"

#include <algorithm>
#include <vector>
#include <ranges>
#include "base/assert.hpp"

namespace minote::sys::vk {

using namespace base;
namespace ranges = std::ranges;

auto createShader(VkDevice device,
	std::span<u32 const> vertSrc, std::span<u32 const> fragSrc) -> Shader {
	Shader result;

	auto const vertCI = VkShaderModuleCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = vertSrc.size_bytes(),
		.pCode = vertSrc.data(),
	};
	auto const fragCI = VkShaderModuleCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = fragSrc.size_bytes(),
		.pCode = fragSrc.data(),
	};
	VK(vkCreateShaderModule(device, &vertCI, nullptr, &result.vert));
	VK(vkCreateShaderModule(device, &fragCI, nullptr, &result.frag));
	return result;
}

void destroyShader(VkDevice device, Shader& shader) {
	vkDestroyShaderModule(device, shader.vert, nullptr);
	vkDestroyShaderModule(device, shader.frag, nullptr);
}

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

}

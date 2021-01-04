#include "sys/vk/shader.hpp"

#include <algorithm>
#include "base/assert.hpp"

namespace minote::sys::vk {

namespace ranges = std::ranges;

auto createShader(VkDevice device,
	std::span<base::u32 const> vertSrc, std::span<base::u32 const> fragSrc,
	std::span<VkDescriptorSetLayoutCreateInfo const> layoutCIs) -> Shader {
	Shader result;
	ASSERT(layoutCIs.size() <= result.descriptorSetLayouts.size());

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

	result.descriptorSetLayouts = {};
	ranges::transform(layoutCIs, result.descriptorSetLayouts.begin(),
		[=](VkDescriptorSetLayoutCreateInfo const& layoutCI) {
			VkDescriptorSetLayout result;
			VK(vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &result));
			return result;
		});

	return result;
}

void destroyShader(VkDevice device, Shader& shader) {
	for (auto layout: shader.descriptorSetLayouts) {
		if (!layout) continue;
		vkDestroyDescriptorSetLayout(device, layout, nullptr);
	}

	vkDestroyShaderModule(device, shader.vert, nullptr);
	vkDestroyShaderModule(device, shader.frag, nullptr);
}

}

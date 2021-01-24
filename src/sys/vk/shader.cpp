#include "sys/vk/shader.hpp"

#include <algorithm>
#include "sys/vk/base.hpp"

namespace minote::sys::vk {

using namespace base;

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

}

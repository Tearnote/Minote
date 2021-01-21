#include "sys/vk/debug.hpp"

#include <string_view>
#include <string>
#include <fmt/core.h>
#include "volk/volk.h"
#include "sys/vk/base.hpp"
#include "base/types.hpp"

namespace minote::sys::vk {

using namespace base;

#ifdef VK_VALIDATION
template<typename T>
static void setDebugNameRaw(VkDevice device, T const handle, VkObjectType type, std::string_view name) {
	auto const sName = std::string{name};
	auto const objectNameInfo = VkDebugUtilsObjectNameInfoEXT{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		.objectType = type,
		.objectHandle = reinterpret_cast<u64>(handle),
		.pObjectName = sName.c_str(),
	};
	VK(vkSetDebugUtilsObjectNameEXT(device, &objectNameInfo));
}
#else //VK_VALIDATION
template<typename T>
static void setDebugNameRaw(VkDevice, T, VkObjectType, std::string_view) {}
#endif //VK_VALIDATION

void setDebugName(VkDevice device, VkInstance instance, std::string_view name) {
	setDebugNameRaw(device, instance, VK_OBJECT_TYPE_INSTANCE, name);
}

void setDebugName(VkDevice device, VkPhysicalDevice physicalDevice, std::string_view name) {
	setDebugNameRaw(device, physicalDevice, VK_OBJECT_TYPE_PHYSICAL_DEVICE, name);
}

void setDebugName(VkDevice device, std::string_view name) {
	setDebugNameRaw(device, device, VK_OBJECT_TYPE_DEVICE, name);
}

void setDebugName(VkDevice device, VkQueue queue, std::string_view name) {
	setDebugNameRaw(device, queue, VK_OBJECT_TYPE_QUEUE, name);
}

void setDebugName(VkDevice device, VkSwapchainKHR swapchain, std::string_view name) {
	setDebugNameRaw(device, swapchain, VK_OBJECT_TYPE_SWAPCHAIN_KHR, name);
}

void setDebugName(VkDevice device, VkCommandPool commandPool, std::string_view name) {
	setDebugNameRaw(device, commandPool, VK_OBJECT_TYPE_COMMAND_POOL, name);
}

void setDebugName(VkDevice device, VkCommandBuffer commandBuffer, std::string_view name) {
	setDebugNameRaw(device, commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER, name);
}

void setDebugName(VkDevice device, VkFence fence, std::string_view name) {
	setDebugNameRaw(device, fence, VK_OBJECT_TYPE_FENCE, name);
}

void setDebugName(VkDevice device, VkSemaphore semaphore, std::string_view name) {
	setDebugNameRaw(device, semaphore, VK_OBJECT_TYPE_SEMAPHORE, name);
}

void setDebugName(VkDevice device, Buffer const& buffer, std::string_view name) {
	setDebugNameRaw(device, buffer.buffer, VK_OBJECT_TYPE_BUFFER, name);
}

void setDebugName(VkDevice device, Image const& image, std::string_view name) {
	setDebugNameRaw(device, image.image, VK_OBJECT_TYPE_IMAGE, fmt::format("{}.image", name));
	setDebugNameRaw(device, image.view, VK_OBJECT_TYPE_IMAGE_VIEW, fmt::format("{}.view", name));
}

void setDebugName(VkDevice device, VkSampler sampler, std::string_view name) {
	setDebugNameRaw(device, sampler, VK_OBJECT_TYPE_SAMPLER, name);
}

void setDebugName(VkDevice device, VkRenderPass renderPass, std::string_view name) {
	setDebugNameRaw(device, renderPass, VK_OBJECT_TYPE_RENDER_PASS, name);
}

void setDebugName(VkDevice device, VkFramebuffer fb, std::string_view name) {
	setDebugNameRaw(device, fb, VK_OBJECT_TYPE_FRAMEBUFFER, name);
}

void setDebugName(VkDevice device, Shader const& shader, std::string_view name) {
	setDebugNameRaw(device, shader.vert, VK_OBJECT_TYPE_SHADER_MODULE, fmt::format("{}.vert", name));
	setDebugNameRaw(device, shader.frag, VK_OBJECT_TYPE_SHADER_MODULE, fmt::format("{}.frag", name));
}

void setDebugName(VkDevice device, VkDescriptorPool descriptorPool, std::string_view name) {
	setDebugNameRaw(device, descriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, name);
}

void setDebugName(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, std::string_view name) {
	setDebugNameRaw(device, descriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, name);
}

void setDebugName(VkDevice device, VkDescriptorSet descriptorSet, std::string_view name) {
	setDebugNameRaw(device, descriptorSet, VK_OBJECT_TYPE_DESCRIPTOR_SET, name);
}

void setDebugName(VkDevice device, VkPipelineLayout layout, std::string_view name) {
	setDebugNameRaw(device, layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, name);
}

void setDebugName(VkDevice device, VkPipeline pipeline, std::string_view name) {
	setDebugNameRaw(device, pipeline, VK_OBJECT_TYPE_PIPELINE, name);
}

}

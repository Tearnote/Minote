#pragma once

#include <string_view>
#include "volk/volk.h"
#include "sys/vk/buffer.hpp"
#include "sys/vk/shader.hpp"
#include "sys/vk/image.hpp"

namespace minote::sys::vk {

void setDebugName(VkDevice device, VkInstance instance, std::string_view name);
void setDebugName(VkDevice device, VkPhysicalDevice physicalDevice, std::string_view name);
void setDebugName(VkDevice device, std::string_view name);
void setDebugName(VkDevice device, VkQueue queue, std::string_view name);
void setDebugName(VkDevice device, VkSwapchainKHR swapchain, std::string_view name);
void setDebugName(VkDevice device, VkCommandPool commandPool, std::string_view name);
void setDebugName(VkDevice device, VkCommandBuffer commandBuffer, std::string_view name);
void setDebugName(VkDevice device, VkFence fence, std::string_view name);
void setDebugName(VkDevice device, VkSemaphore semaphore, std::string_view name);
void setDebugName(VkDevice device, Buffer const& buffer, std::string_view name);
void setDebugName(VkDevice device, Image const& image, std::string_view name);
void setDebugName(VkDevice device, VkSampler sampler, std::string_view name);
void setDebugName(VkDevice device, VkRenderPass renderPass, std::string_view name);
void setDebugName(VkDevice device, VkFramebuffer fb, std::string_view name);
void setDebugName(VkDevice device, Shader const& shader, std::string_view name);
void setDebugName(VkDevice device, VkDescriptorPool descriptorPool, std::string_view name);
void setDebugName(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, std::string_view name);
void setDebugName(VkDevice device, VkDescriptorSet descriptorSet, std::string_view name);
void setDebugName(VkDevice device, VkPipelineLayout layout, std::string_view name);
void setDebugName(VkDevice device, VkPipeline pipeline, std::string_view name);

}

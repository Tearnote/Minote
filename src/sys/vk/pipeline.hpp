#pragma once

#include <vector>
#include <span>
#include "volk/volk.h"
#include "base/types.hpp"
#include "sys/vk/base.hpp"

namespace minote::sys::vk {

using namespace base;

struct PipelineBuilder {

	std::vector<VkPipelineShaderStageCreateInfo> shaderStageCIs;
	VkPipelineVertexInputStateCreateInfo vertexInputStateCI;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI;
	VkPipelineRasterizationStateCreateInfo rasterizationStateCI;
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState;
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCI;
	VkPipelineMultisampleStateCreateInfo multisampleStateCI;
	VkPipelineLayout layout;

	auto build(VkDevice device, VkRenderPass pass, u32 subpass = 0) -> VkPipeline;

};

constexpr auto makePipelineShaderStageCI(VkShaderStageFlagBits stage,
	VkShaderModule shaderModule) {
	return VkPipelineShaderStageCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = stage,
		.module = shaderModule,
		.pName = "main",
	};
}

constexpr auto makePipelineVertexInputStateCI(
	std::span<VkVertexInputBindingDescription const> vertexBindings = {},
	std::span<VkVertexInputAttributeDescription const> vertexAttributes = {}) {
	return VkPipelineVertexInputStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = static_cast<u32>(vertexBindings.size()),
		.pVertexBindingDescriptions = vertexBindings.data(),
		.vertexAttributeDescriptionCount = static_cast<u32>(vertexAttributes.size()),
		.pVertexAttributeDescriptions = vertexAttributes.data(),
	};
}

constexpr auto makePipelineInputAssemblyStateCI(VkPrimitiveTopology topology) {
	return VkPipelineInputAssemblyStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = topology,
		.primitiveRestartEnable = VK_FALSE,
	};
}

constexpr auto makePipelineRasterizationStateCI(VkPolygonMode polygonMode, bool culling) {
	return VkPipelineRasterizationStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = polygonMode,
		.cullMode = culling? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp = 0.0f,
		.depthBiasSlopeFactor = 0.0f,
		.lineWidth = 1.0f,
	};
}

constexpr auto makePipelineMultisampleStateCI(VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT) {
	return VkPipelineMultisampleStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = sampleCount,
	};
}

constexpr auto makePipelineDepthStencilStateCI(bool depthTest, bool depthWrite, VkCompareOp depthOp) {
	return VkPipelineDepthStencilStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = static_cast<VkBool32>(depthTest? VK_TRUE : VK_FALSE),
		.depthWriteEnable = static_cast<VkBool32>(depthWrite? VK_TRUE : VK_FALSE),
		.depthCompareOp = depthTest? depthOp : VK_COMPARE_OP_ALWAYS,
		.stencilTestEnable = VK_FALSE,
	};
}

constexpr auto makePipelineColorBlendAttachmentState(bool alphaBlending) {
	if (alphaBlending) {
		return VkPipelineColorBlendAttachmentState{
			.blendEnable = VK_TRUE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
				VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		};
	} else {
		return VkPipelineColorBlendAttachmentState{
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
				VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		};
	}
}

inline auto createPipelineLayout(VkDevice device, std::span<VkDescriptorSetLayout const> descriptorSetLayouts = {}, std::span<VkPushConstantRange const> pushConstants = {}) {
	auto pipelineLayoutCI = VkPipelineLayoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.flags = 0,
		.setLayoutCount = static_cast<u32>(descriptorSetLayouts.size()),
		.pSetLayouts = descriptorSetLayouts.data(),
		.pushConstantRangeCount = static_cast<u32>(pushConstants.size()),
		.pPushConstantRanges = pushConstants.data(),
	};
	VkPipelineLayout result;
	VK(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &result));
	return result;
}

}

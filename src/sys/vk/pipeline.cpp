#include "sys/vk/pipeline.hpp"

#include "base/types.hpp"
#include "sys/vk/base.hpp"

namespace minote::sys::vk {

using namespace base;

auto PipelineBuilder::build(VkDevice device, VkRenderPass pass) -> VkPipeline {
	auto const viewportStateCI = VkPipelineViewportStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1,
	};
	auto const colorBlendStateCI = VkPipelineColorBlendStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachmentState,
	};
	auto const dynamicStates = std::to_array<VkDynamicState>({
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	});
	auto const dynamicStateCI = VkPipelineDynamicStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = dynamicStates.size(),
		.pDynamicStates = dynamicStates.data(),
	};

	auto const pipelineCI = VkGraphicsPipelineCreateInfo{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = static_cast<u32>(shaderStageCIs.size()),
		.pStages = shaderStageCIs.data(),
		.pVertexInputState = &vertexInputStateCI,
		.pInputAssemblyState = &inputAssemblyStateCI,
		.pViewportState = &viewportStateCI,
		.pRasterizationState = &rasterizationStateCI,
		.pMultisampleState = &multisampleStateCI,
		.pDepthStencilState = &depthStencilStateCI,
		.pColorBlendState = &colorBlendStateCI,
		.pDynamicState = &dynamicStateCI,
		.layout = layout,
		.renderPass = pass,
	};

	VkPipeline result;
	VK(vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineCI, nullptr, &result));
	return result;
}

}
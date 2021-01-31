#include "sys/vk/pipeline.hpp"

#include "base/types.hpp"
#include "sys/vk/base.hpp"

namespace minote::sys::vk {

using namespace base;

auto PipelineBuilder::build(VkDevice device, VkRenderPass pass, u32 subpass) -> VkPipeline {
	auto const shaderStageCIs = std::array{
		makePipelineShaderStageCI(VK_SHADER_STAGE_VERTEX_BIT, shader.vert),
		makePipelineShaderStageCI(VK_SHADER_STAGE_FRAGMENT_BIT, shader.frag),
	};
	auto const viewportStateCI = VkPipelineViewportStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1,
	};
	auto const multisampleStateCI = VkPipelineMultisampleStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	};
	auto const colorBlendStateCI = VkPipelineColorBlendStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachmentState,
	};
	auto const dynamicStates = std::array{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};
	auto const dynamicStateCI = VkPipelineDynamicStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = dynamicStates.size(),
		.pDynamicStates = dynamicStates.data(),
	};

	auto const pipelineCI = VkGraphicsPipelineCreateInfo{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = shaderStageCIs.size(),
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
		.subpass = subpass,
	};

	VkPipeline result;
	VK(vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineCI, nullptr, &result));
	return result;
}

}

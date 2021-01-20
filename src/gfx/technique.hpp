#pragma once

#include <array>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include "VulkanMemoryAllocator/vma.h"
#include "volk/volk.h"
#include "base/hashmap.hpp"
#include "base/id.hpp"
#include "sys/vk/shader.hpp"
#include "gfx/indirect.hpp"
#include "gfx/base.hpp"

namespace minote::gfx {

using namespace base;

// A collection of pipelines for object drawing. While all materials are drawn with a single
// ubershader, multiple pipelines are still required to handle different requirements such as
// rasterizer state. These pipelines will be called techniques, as every material can
// be rendered using every technique.
// The shader and all relevant buffers are owned by this object.
struct TechniqueSet {

	struct Technique {

		VkPipeline pipeline;
		PerFrame<VkDescriptorSet> drawDescriptorSet;
		PerFrame<IndirectBuffer> indirect;

		auto getDescriptorSet(i64 frameIndex) -> VkDescriptorSet& { return drawDescriptorSet[frameIndex]; }

	};

	void create(VkDevice device, VkDescriptorSetLayout worldLayout);

	void destroy(VkDevice device, VmaAllocator allocator);

	void addTechnique(ID id, VkDevice device, VmaAllocator allocator, VkRenderPass renderPass,
		VkDescriptorPool descriptorPool, PerFrame<VkDescriptorSet> worldDescriptorSets,
		VkPipelineRasterizationStateCreateInfo rasterizationStateCI,
		VkPipelineColorBlendAttachmentState colorBlendAttachmentState,
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI,
		VkPipelineMultisampleStateCreateInfo multisampleStateCI);

	auto getPipelineLayout() { return m_pipelineLayout; }

	auto getTechnique(ID id) -> Technique& { return m_techniques.at(id); }

	auto getTechniqueIndirect(ID id, i64 frameIndex) -> IndirectBuffer& {
		return getTechnique(id).indirect[frameIndex];
	}

private:

	sys::vk::Shader m_shader;
	VkDescriptorSetLayout m_drawDescriptorSetLayout;
	VkPipelineLayout m_pipelineLayout;
	hashmap<ID, Technique> m_techniques;

};

}

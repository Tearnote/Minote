#pragma once

#include <array>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include "VulkanMemoryAllocator/vma.h"
#include "volk/volk.h"
#include "base/hashmap.hpp"
#include "base/id.hpp"
#include "sys/vk/shader.hpp"
#include "sys/vk/buffer.hpp"
#include "gfx/indirect.hpp"
#include "gfx/base.hpp"
#include "gfx/mesh.hpp"

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
		PerFrame<std::array<VkDescriptorSet, 2>> descriptorSets;
		PerFrame<IndirectBuffer> indirect;

	};

	void create(VkDevice device, VmaAllocator allocator, VkDescriptorPool descriptorPool, MeshBuffer& meshBuffer);

	void destroy(VkDevice device, VmaAllocator allocator);

	void addTechnique(ID id, VkDevice device, VmaAllocator allocator,
		VkDescriptorPool descriptorPool, VkRenderPass renderPass,
		VkPipelineRasterizationStateCreateInfo rasterizationStateCI,
		VkPipelineColorBlendAttachmentState colorBlendAttachmentState,
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI,
		VkPipelineMultisampleStateCreateInfo multisampleStateCI);

	auto getPipelineLayout() { return m_pipelineLayout; }

	auto getWorldConstants(i64 frameIndex) -> sys::vk::Buffer& {
		return m_worldConstants[frameIndex];
	}

	auto getTechnique(ID id) -> Technique& { return m_techniques.at(id); }

	auto getTechniqueIndirect(ID id, i64 frameIndex) -> IndirectBuffer& {
		return getTechnique(id).indirect[frameIndex];
	}

private:

	sys::vk::Shader m_shader;
	PerFrame<VkDescriptorSet> m_worldDescriptorSet;
	PerFrame<sys::vk::Buffer> m_worldConstants;
	VkPipelineLayout m_pipelineLayout;
	hashmap<ID, Technique> m_techniques;

};

}

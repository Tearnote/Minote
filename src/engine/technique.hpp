#pragma once

#include <array>
#include "VulkanMemoryAllocator/vma.h"
#include "volk/volk.h"
#include "base/hashmap.hpp"
#include "base/id.hpp"
#include "sys/vk/shader.hpp"
#include "sys/vk/buffer.hpp"
#include "engine/indirect.hpp"
#include "engine/base.hpp"
#include "engine/mesh.hpp"

namespace minote::engine {

// A collection of pipelines for object drawing. While all materials are drawn with a single
// ubershader, multiple pipelines are still required to handle different requirements such as
// rasterizer state. These pipelines will be called techniques, as every material can
// be rendered using every technique.
// The shader and all relevant buffers are owned by this object.
struct TechniqueSet {

	struct World {

		glm::mat4 view;
		glm::mat4 projection;
		glm::mat4 viewProjection;

	};

	struct Technique {

		VkPipeline pipeline;
		PerFrame<VkDescriptorSet> drawDescriptorSet;
		PerFrame<std::array<VkDescriptorSet, 2>> descriptorSets;
		PerFrame<IndirectBuffer> indirect;

	};

	void create(VkDevice device, VmaAllocator allocator, VkDescriptorPool descriptorPool, MeshBuffer& meshBuffer);

	void destroy(VkDevice device, VmaAllocator allocator);

	void addTechnique(base::ID id, VkDevice device, VmaAllocator allocator,
		VkDescriptorPool descriptorPool, VkRenderPass renderPass,
		VkPipelineRasterizationStateCreateInfo rasterizationStateCI,
		VkPipelineColorBlendAttachmentState colorBlendAttachmentState,
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI,
		VkPipelineMultisampleStateCreateInfo multisampleStateCI);

	auto getPipelineLayout() { return m_pipelineLayout; }

	auto getWorldConstants(base::i64 frameIndex) -> sys::vk::Buffer& {
		return m_worldConstants[frameIndex];
	}

	auto getTechnique(base::ID id) -> Technique& { return m_techniques.at(id); }

	auto getTechniqueIndirect(base::ID id, base::i64 frameIndex) -> IndirectBuffer& {
		return getTechnique(id).indirect[frameIndex];
	}

private:

	sys::vk::Shader m_shader;
	PerFrame<VkDescriptorSet> m_worldDescriptorSet;
	PerFrame<sys::vk::Buffer> m_worldConstants;
	VkPipelineLayout m_pipelineLayout;
	base::hashmap<base::ID, Technique> m_techniques;

};

}

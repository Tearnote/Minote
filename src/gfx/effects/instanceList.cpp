#include "gfx/effects/instanceList.hpp"

#include "vuk/AllocatorHelpers.hpp"
#include "vuk/CommandBuffer.hpp"
#include "vuk/RenderGraph.hpp"
#include "vuk/Partials.hpp"
#include "gfx/renderer.hpp"
#include "gfx/samplers.hpp"
#include "gfx/shader.hpp"
#include "sys/vulkan.hpp"

namespace minote {

InstanceList::InstanceList(vuk::Allocator& _allocator, ModelBuffer& _models, ObjectBuffer& _objects) {
	
	compile();
	
	instanceBound = _objects.meshletCount;

	auto rg = std::make_shared<vuk::RenderGraph>("instanceList");
	rg->attach_in("models", _models.models);
	rg->attach_in("modelIndices", _objects.modelIndices);
	rg->attach_buffer("instances", vuk::Buffer{
		.size = instanceBound * sizeof(Instance),
		.memory_usage = vuk::MemoryUsage::eGPUonly,
	});
	rg->attach_buffer("instanceCount", vuk::Buffer{
		.size = sizeof(uint4),
		.memory_usage = vuk::MemoryUsage::eCPUtoGPU,
	});
	rg->add_pass(vuk::Pass{
		.name = "instanceList/genInstances",
		.resources = {
			"models"_buffer >> vuk::eComputeRead,
			"modelIndices"_buffer >> vuk::eComputeRead,
			"instanceCount"_buffer >> vuk::eComputeRW >> "instanceCount/final",
			"instances"_buffer >> vuk::eComputeWrite >> "instances/final",
		},
		.execute = [&_objects](vuk::CommandBuffer& cmd) {
			
			cmd.bind_compute_pipeline("instanceList/genInstances")
			   .bind_buffer(0, 0, "models")
			   .bind_buffer(0, 1, "modelIndices")
			   .bind_buffer(0, 2, "instanceCount")
			   .bind_buffer(0, 3, "instances");

			auto countInitial = uint4{0, 1, 1, 0};
			auto count = *cmd.get_resource_buffer("instanceCount");
			std::memcpy(count.mapped_ptr, &countInitial, sizeof(countInitial));
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, _objects.objectCount);
			
			cmd.dispatch_invocations(_objects.objectCount);
			
		},
	});
	
	instanceCount = vuk::Future(rg, "instanceCount/final");
	instances = vuk::Future(rg, "instances/final");
	triangleBound = _objects.triangleCount;
	
}

GET_SHADER(instanceList_genInstances_cs);
void InstanceList::compile() {
	
	if (m_compiled) return;
	auto& ctx = *s_vulkan->context;
	
	auto genInstancesPci = vuk::PipelineBaseCreateInfo();
	ADD_SHADER(genInstancesPci, instanceList_genInstances_cs, "instanceList/genInstances.cs.hlsl");
	ctx.create_named_pipeline("instanceList/genInstances", genInstancesPci);
	
	m_compiled = true;
	
}

TriangleList::TriangleList(vuk::Allocator& _allocator, ModelBuffer& _models, InstanceList& _instances) {
	
	compile();
	
	auto rg = std::make_shared<vuk::RenderGraph>("triangleList");
	rg->attach_in("meshlets", _models.meshlets);
	rg->attach_in("triIndices", _models.triIndices);
	rg->attach_in("instanceCount", _instances.instanceCount);
	rg->attach_in("instances", _instances.instances);
	rg->attach_buffer("command", vuk::Buffer{
		.size = sizeof(Command),
		.memory_usage = vuk::MemoryUsage::eCPUtoGPU,
	});
	rg->attach_buffer("indices", vuk::Buffer{
		.size = _instances.triangleBound * 3 * sizeof(uint),
		.memory_usage = vuk::MemoryUsage::eGPUonly,
	});
	rg->add_pass(vuk::Pass{
		.name = "instanceList/genIndices",
		.resources = {
			"meshlets"_buffer >> vuk::eComputeRead,
			"triIndices"_buffer >> vuk::eComputeRead,
			"instanceCount"_buffer >> vuk::eIndirectRead,
			"instances"_buffer >> vuk::eComputeRead,
			"command"_buffer >> vuk::eComputeRW >> "command/final",
			"indices"_buffer >> vuk::eComputeWrite >> "indices/final",
		},
		.execute = [instanceCount=_instances.instanceCount](vuk::CommandBuffer& cmd) {
			
			cmd.bind_compute_pipeline("instanceList/genIndices")
			   .bind_buffer(0, 0, "meshlets")
			   .bind_buffer(0, 1, "triIndices")
			   .bind_buffer(0, 2, "instanceCount")
			   .bind_buffer(0, 3, "instances")
			   .bind_buffer(0, 4, "command")
			   .bind_buffer(0, 5, "indices");

			auto command = *cmd.get_resource_buffer("command");
			auto commandInitial = Command{
				.indexCount = 0, // Calculated at runtime
				.instanceCount = 1,
				.firstIndex = 0,
				.vertexOffset = 0,
				.firstInstance = 0,
			};
			std::memcpy(command.mapped_ptr, &commandInitial, sizeof(commandInitial));
			
			cmd.specialize_constants(0, MeshletMaxTris);
			
			cmd.dispatch_indirect("instanceCount");
			
		},
	});
	
	command = vuk::Future(rg, "command/final");
	indices = vuk::Future(rg, "indices/final");
	
}

GET_SHADER(instanceList_genIndices_cs);
void TriangleList::compile() {
	
	if (m_compiled) return;
	auto& ctx = *s_vulkan->context;
	
	auto genIndicesPci = vuk::PipelineBaseCreateInfo();
	ADD_SHADER(genIndicesPci, instanceList_genIndices_cs, "instanceList/genIndices.cs.hlsl");
	ctx.create_named_pipeline("instanceList/genIndices", genIndicesPci);
	
	m_compiled = true;
	
}

}

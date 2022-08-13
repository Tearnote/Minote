#include "gfx/effects/instanceList.hpp"

#include "vuk/AllocatorHelpers.hpp"
#include "vuk/CommandBuffer.hpp"
#include "vuk/RenderGraph.hpp"
#include "vuk/Partials.hpp"
#include "gfx/renderer.hpp"
#include "gfx/shader.hpp"
#include "sys/vulkan.hpp"

namespace minote {

InstanceList::InstanceList(vuk::Allocator& _allocator, ModelBuffer& _models, ObjectBuffer& _objects):
	instanceCount(0) {
	
	compile();
	
	auto instancesBuf = *vuk::allocate_buffer_gpu(_allocator, vuk::BufferCreateInfo{
		.mem_usage = vuk::MemoryUsage::eGPUonly,
		.size = _objects.meshletCount * sizeof(Instance),
	});
	auto cpu_instanceCount = 0u;
	auto instanceCountBuf = vuk::create_buffer_cross_device(s_renderer->frameAllocator(),
		vuk::MemoryUsage::eCPUtoGPU, span(&cpu_instanceCount, 1)).second;
	auto rg = std::make_shared<vuk::RenderGraph>("instanceList");
	
	rg->attach_in("models", _models.models);
	rg->attach_in("modelIndices", _objects.modelIndices);
	rg->attach_buffer("instances", *instancesBuf); //TODO convert to managed buffer, pending vuk feature
	rg->attach_in("instanceCount", instanceCountBuf);
	rg->add_pass(vuk::Pass{
		.name = "instanceList/genInstances",
		.resources = {
			"models"_buffer >> vuk::eComputeRead,
			"modelIndices"_buffer >> vuk::eComputeRead,
			"instances"_buffer >> vuk::eComputeWrite >> "instances/final",
			"instanceCount"_buffer >> vuk::eComputeRW >> "instanceCount/final",
		},
		.execute = [&_objects](vuk::CommandBuffer& cmd) {
			
			cmd.bind_compute_pipeline("instanceList/genInstances")
			   .bind_buffer(0, 0, "models")
			   .bind_buffer(0, 1, "modelIndices")
			   .bind_buffer(0, 2, "instances")
			   .bind_buffer(0, 3, "instanceCount");
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, _objects.objectCount);
			
			cmd.dispatch_invocations(_objects.objectCount);
			
		},
	});
	
	instances = vuk::Future(rg, "instances/final");
	instanceCount = _objects.meshletCount;
	triangleCount = _objects.triangleCount;
	
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

TriangleList::TriangleList(vuk::Allocator& _allocator,
	ModelBuffer& _models, InstanceList& _instances) {
	
	compile();
	
	auto commandData = Command{
		.indexCount = 0, // Calculated at runtime
		.instanceCount = 1,
		.firstIndex = 0,
		.vertexOffset = 0,
		.firstInstance = 0,
	};
	auto command = vuk::create_buffer_cross_device(s_renderer->frameAllocator(),
		vuk::MemoryUsage::eCPUtoGPU, span(&commandData, 1)).second;
	
	auto indicesBuf = *vuk::allocate_buffer_gpu(_allocator, vuk::BufferCreateInfo{
		.mem_usage = vuk::MemoryUsage::eGPUonly,
		.size = _instances.triangleCount * 3 * sizeof(uint),
	});
	auto rg = std::make_shared<vuk::RenderGraph>("triangleList");
	rg->attach_in("meshlets", _models.meshlets);
	rg->attach_in("triIndices", _models.triIndices);
	rg->attach_in("instances", _instances.instances);
	rg->attach_in("command", command);
	rg->attach_buffer("indices", *indicesBuf); //TODO convert to managed buffer, pending vuk feature
	rg->add_pass(vuk::Pass{
		.name = "instanceList/genIndices",
		.resources = {
			"meshlets"_buffer >> vuk::eComputeRead,
			"triIndices"_buffer >> vuk::eComputeRead,
			"instances"_buffer >> vuk::eComputeRead,
			"command"_buffer >> vuk::eComputeRW >> "command/final",
			"indices"_buffer >> vuk::eComputeWrite >> "indices/final",
		},
		.execute = [instanceCount=_instances.instanceCount](vuk::CommandBuffer& cmd) {
			
			cmd.bind_compute_pipeline("instanceList/genIndices")
			   .bind_buffer(0, 0, "meshlets")
			   .bind_buffer(0, 1, "triIndices")
			   .bind_buffer(0, 2, "instances")
			   .bind_buffer(0, 3, "command")
			   .bind_buffer(0, 4, "indices");
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, instanceCount);
			cmd.specialize_constants(0, MeshletMaxTris);
			
			cmd.dispatch_invocations(instanceCount);
			
		}});
	
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

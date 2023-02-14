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

auto InstanceList::cull(ModelBuffer& _models, ObjectBuffer& _objects,
	float4x4 _view, float4x4 _projection) -> InstanceList {

	compile();

	auto rg = std::make_shared<vuk::RenderGraph>("instanceList/cull");
	rg->attach_in("meshlets", _models.meshlets);
	rg->attach_in("transforms", _objects.transforms);
	rg->attach_in("instanceCount", instanceCount);
	rg->attach_in("instances", instances);
	rg->attach_buffer("outInstanceCount", vuk::Buffer{
		.size = sizeof(uint4),
		.memory_usage = vuk::MemoryUsage::eCPUtoGPU,
	});
	rg->attach_buffer("outInstances", vuk::Buffer{
		.memory_usage = vuk::MemoryUsage::eGPUonly,
	});
	rg->inference_rule("outInstances", vuk::same_size_as("instances"));
	rg->attach_image("stub", vuk::ImageAttachment{ // Placeholder for the HiZ binding
		.extent = vuk::Dimension3D::absolute(1, 1),
		.format = vuk::Format::eR8Unorm,
		.sample_count = vuk::Samples::e1,
		.level_count = 1,
		.layer_count = 1,
	});

	auto resources = std::vector<vuk::Resource>{
		"meshlets"_buffer >> vuk::eComputeRead,
		"transforms"_buffer >> vuk::eComputeRead,
		"instanceCount"_buffer >> vuk::eIndirectRead,
		"instances"_buffer >> vuk::eComputeRead,
		"outInstanceCount"_buffer >> vuk::eComputeRW >> "outInstanceCount/final",
		"outInstances"_buffer >> vuk::eComputeWrite >> "outInstances/final",
	};
	resources.emplace_back("stub"_image >> vuk::eComputeSampled);
	rg->add_pass(vuk::Pass{
		.name = "instanceList/cull",
		.resources = std::move(resources),
		.execute = [_view, _projection](vuk::CommandBuffer& cmd) {
			
			cmd.bind_compute_pipeline("instanceList/cull")
			   .bind_buffer(0, 0, "meshlets")
			   .bind_buffer(0, 1, "transforms")
			   .bind_buffer(0, 2, "instanceCount")
			   .bind_buffer(0, 3, "instances")
			   .bind_buffer(0, 4, "outInstanceCount")
			   .bind_buffer(0, 5, "outInstances");
			cmd.bind_image(0, 6, "stub").bind_sampler(0, 6, MinClamp);

			auto outCount = *cmd.get_resource_buffer("outInstanceCount");
			auto outCountInitial = uint4{0, 1, 1, 0};
			std::memcpy(outCount.mapped_ptr, &outCountInitial, sizeof(outCountInitial));
			
			struct Constants {
				float4x4 view;
				float4 frustum;
				float P00;
				float P11;
			};
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, Constants{
				.view = _view,
				.frustum = [_projection]() {
					float4 frustumX = _projection[3] + _projection[0];
					float4 frustumY = _projection[3] + _projection[1];
					frustumX /= length(float3(frustumX));
					frustumY /= length(float3(frustumY));
					return float4{frustumX.x(), frustumX.z(), frustumY.y(), frustumY.z()};
				}(),
				.P00 = _projection[0][0],
				.P11 = _projection[1][1],
			});
			
			cmd.specialize_constants(0, 0);
			cmd.specialize_constants(1, _projection[2][3]);
			cmd.dispatch_indirect("instanceCount");
			
		},
	});

	auto result = InstanceList();
	result.instanceCount = vuk::Future(rg, "outInstanceCount/final");
	result.instances = vuk::Future(rg, "outInstances/final");
	result.instanceBound = instanceBound;
	result.triangleBound = triangleBound;
	return result;
	
}

GET_SHADER(instanceList_genInstances_cs);
GET_SHADER(instanceList_cull_cs);
void InstanceList::compile() {
	
	if (m_compiled) return;
	auto& ctx = *s_vulkan->context;
	
	auto genInstancesPci = vuk::PipelineBaseCreateInfo();
	ADD_SHADER(genInstancesPci, instanceList_genInstances_cs, "instanceList/genInstances.cs.hlsl");
	ctx.create_named_pipeline("instanceList/genInstances", genInstancesPci);

	auto cullPci = vuk::PipelineBaseCreateInfo();
	ADD_SHADER(cullPci, instanceList_cull_cs, "instanceList/cull.cs.hlsl");
	ctx.create_named_pipeline("instanceList/cull", cullPci);
	
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

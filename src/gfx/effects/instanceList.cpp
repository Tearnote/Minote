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
	auto instanceCount = vuk::create_buffer_cross_device(s_renderer->frameAllocator(),
		vuk::MemoryUsage::eCPUtoGPU, span(&cpu_instanceCount, 1)).second;
	auto rg = std::make_shared<vuk::RenderGraph>("instanceList");
	
	rg->attach_in("models", _models.models);
	rg->attach_in("modelIndices", _objects.modelIndices);
	rg->attach_buffer("instances", *instancesBuf); //TODO convert to managed buffer, pending vuk feature
	rg->attach_in("instanceCount", instanceCount);
	rg->add_pass(vuk::Pass{
		.name = "instanceList/expandObjects",
		.resources = {
			"models"_buffer >> vuk::eComputeRead,
			"modelIndices"_buffer >> vuk::eComputeRead,
			"instances"_buffer >> vuk::eComputeWrite >> "instances/final",
			"instanceCount"_buffer >> vuk::eComputeRW >> "instanceCount/final",
		},
		.execute = [&_objects](vuk::CommandBuffer& cmd) {
			
			cmd.bind_compute_pipeline("instanceList/expandObjects")
			   .bind_buffer(0, 0, "models")
			   .bind_buffer(0, 1, "modelIndices")
			   .bind_buffer(0, 2, "instances")
			   .bind_buffer(0, 3, "instanceCount");
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, _objects.objectCount);
			
			cmd.dispatch_invocations(_objects.objectCount);
			
		},
	});
	
	instances = vuk::Future(rg, "instances/final");
	
}

GET_SHADER(instanceList_expandObjects_cs);
void InstanceList::compile() {
	
	if (m_compiled) return;
	auto& ctx = *s_vulkan->context;
	
	auto expandObjectsPci = vuk::PipelineBaseCreateInfo();
	ADD_SHADER(expandObjectsPci, instanceList_expandObjects_cs, "instanceList/expandObjects.cs.hlsl");
	ctx.create_named_pipeline("instanceList/expandObjects", expandObjectsPci);
	
	m_compiled = true;
	
}

/*
void TriangleList::compile(vuk::PerThreadContext& _ptc) {
	
	auto genIndicesPci = vuk::ComputePipelineBaseCreateInfo();
	genIndicesPci.add_spirv(std::vector<uint>{
#include "spv/instanceList/genIndices.comp.spv"
	}, "instanceList/genIndices.comp");
	_ptc.ctx.create_named_pipeline("instanceList/genIndices", genIndicesPci);
	
	auto cullMeshletsPci = vuk::ComputePipelineBaseCreateInfo();
	cullMeshletsPci.add_spirv(std::vector<uint>{
#include "spv/instanceList/cullMeshlets.comp.spv"
	}, "instanceList/cullMeshlets.comp");
	_ptc.ctx.create_named_pipeline("instanceList/cullMeshlets", cullMeshletsPci);
	
}

auto TriangleList::fromInstances(InstanceList _instances, Pool& _pool, Frame& _frame, vuk::Name _name,
	Texture2D _hiZ, uint2 _hiZInnerSize, float4x4 _view, float4x4 _projection) -> TriangleList {
	
	auto result = TriangleList();
	result.colors = _instances.colors;
	result.transforms = _instances.transforms;
	result.prevTransforms = _instances.prevTransforms;
	
	result.instances = Buffer<Instance>::make(_pool, nameAppend(_name, "instances"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		_instances.size());
	result.instances.attach(_frame.rg, vuk::eNone, vuk::eNone);
	
	auto instanceCountData = uint4{0, 1, 1, 0};
	result.instanceCount = Buffer<uint4>::make(_frame.framePool, nameAppend(_name, "instanceCount"),
		vuk::BufferUsageFlagBits::eIndirectBuffer |
		vuk::BufferUsageFlagBits::eStorageBuffer,
		std::span(&instanceCountData, 1));
	result.instanceCount.attach(_frame.rg, vuk::eHostWrite, vuk::eNone);
	
	auto groupCounterData = 0u;
	auto groupCounter = Buffer<uint>::make(_frame.framePool, nameAppend(_name, "groupCounter"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		std::span(&groupCounterData, 1));
	
	auto invView = inverse(_view);
	auto cameraPos = float3{invView[3][0], invView[3][1], invView[3][2]};
	
	_frame.rg.add_pass({
		.name = nameAppend(_name, "instanceList/cullMeshlets"),
		.resources = {
			_instances.instances.resource(vuk::eComputeRead),
			_instances.transforms.resource(vuk::eComputeRead),
			_hiZ.resource(vuk::eComputeSampled),
			result.instanceCount.resource(vuk::eComputeRW),
			result.instances.resource(vuk::eComputeWrite) },
		.execute = [&_frame, _instances, result, groupCounter, _view,
			_hiZ, _hiZInnerSize, _projection](vuk::CommandBuffer& cmd) {
			
			cmd.bind_storage_buffer(0, 1, _frame.models.meshlets)
			   .bind_storage_buffer(0, 2, _instances.instances)
			   .bind_storage_buffer(0, 3, _instances.transforms)
			   .bind_sampled_image(0, 4, _hiZ, MinClamp)
			   .bind_storage_buffer(0, 5, result.instanceCount)
			   .bind_storage_buffer(0, 6, result.instances)
			   .bind_storage_buffer(0, 7, groupCounter)
			   .bind_compute_pipeline("instanceList/cullMeshlets");
			
			struct CullingData {
				float4x4 view;
				float4 frustum;
				float P00;
				float P11;
			};
			*cmd.map_scratch_uniform_binding<CullingData>(0, 0) = CullingData{
				.view = _view,
				.frustum = [_projection] {
					
					auto projectionT = transpose(_projection);
					float4 frustumX = projectionT[3] + projectionT[0];
					float4 frustumY = projectionT[3] + projectionT[1];
					frustumX /= length(float3(frustumX));
					frustumY /= length(float3(frustumY));
					return float4{frustumX.x(), frustumX.z(), frustumY.y(), frustumY.z()};
					
				}(),
				.P00 = _projection[0][0],
				.P11 = _projection[1][1] };
			
			cmd.specialize_constants(0, MeshletMaxTris);
			cmd.specialize_constants(1, _projection[3][2]);
			cmd.specialize_constants(2, uintFromuint16(_hiZ.size()));
			cmd.specialize_constants(3, uintFromuint16(_hiZInnerSize));
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, uint(_instances.size()));
			
			cmd.dispatch_invocations(_instances.size());
			
		}});
	
	auto commandData = Command({
		.indexCount = 0, // Calculated at runtime
		.instanceCount = 1,
		.firstIndex = 0,
		.vertexOffset = 0,
		.firstInstance = 0 });
	result.command = Buffer<Command>::make(_frame.framePool, nameAppend(_name, "command"),
		vuk::BufferUsageFlagBits::eIndirectBuffer |
		vuk::BufferUsageFlagBits::eStorageBuffer,
		std::span(&commandData, 1));
	result.command.attach(_frame.rg, vuk::eHostWrite, vuk::eNone);
	
	result.indices = Buffer<uint>::make(_pool, _name,
		vuk::BufferUsageFlagBits::eIndexBuffer |
		vuk::BufferUsageFlagBits::eStorageBuffer,
		_instances.triangleCount * 3);
	result.indices.attach(_frame.rg, vuk::eNone, vuk::eNone);
	
	_frame.rg.add_pass({
		.name = nameAppend(_name, "instanceList/genIndices"),
		.resources = {
			result.instanceCount.resource(vuk::eIndirectRead),
			result.instances.resource(vuk::eComputeRead),
			result.transforms.resource(vuk::eComputeRead),
			result.command.resource(vuk::eComputeRW),
			result.indices.resource(vuk::eComputeWrite) },
		.execute = [result, &_frame, _view, cameraPos](vuk::CommandBuffer& cmd) {
			
			cmd.bind_storage_buffer(0, 0, _frame.models.meshlets)
			   .bind_storage_buffer(0, 1, result.instances)
			   .bind_storage_buffer(0, 2, result.instanceCount)
			   .bind_storage_buffer(0, 3, result.transforms)
			   .bind_storage_buffer(0, 4, _frame.models.triIndices)
			   .bind_storage_buffer(0, 5, _frame.models.vertIndices)
			   .bind_storage_buffer(0, 6, _frame.models.vertices)
			   .bind_storage_buffer(0, 7, result.command)
			   .bind_storage_buffer(0, 8, result.indices)
			   .bind_compute_pipeline("instanceList/genIndices");
			
			cmd.specialize_constants(0, MeshletMaxTris);
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, cameraPos);
			
			cmd.dispatch_indirect(result.instanceCount);
			
		}});
	
	return result;
	
}
*/
}

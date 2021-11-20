#include "gfx/effects/instanceList.hpp"

#include <cassert>
#include "Tracy.hpp"
#include "base/containers/vector.hpp"
#include "base/util.hpp"
#include "gfx/util.hpp"

namespace minote::gfx {

using namespace base;

auto BasicInstanceList::upload(Pool& _pool, vuk::RenderGraph& _rg, vuk::Name _name,
	ObjectPool const& _objects, ModelBuffer const& _models) -> BasicInstanceList {
	
	ZoneScoped;
	
	auto instances = pvector<Instance>();
	auto colors = pvector<vec4>();
	auto basicTransforms = pvector<BasicTransform>();
	
	auto instancesCount = 0u;
	
	// Queue up meshes of all valid objects
	
	for (auto id: iota(ObjectID(0), _objects.size())) {
		
		auto& metadata = _objects.metadata[id];
		if (!metadata.exists || !metadata.visible)
			continue;
		
		auto modelID = _objects.modelIDs[id];
		for (auto meshIdx: _models.cpu_modelMeshes.at(modelID)) {
			
			instances.emplace_back(Instance{
				.meshIdx = u32(meshIdx),
				.transformIdx = instancesCount });
			colors.emplace_back(_objects.colors[id]);
			basicTransforms.emplace_back(_objects.transforms[id]);
			
			instancesCount += 1;
			
		}
		
	}
	
	// Upload data to GPU
	
	auto result = BasicInstanceList();
	
	auto instancesCountGroups = instancesCount / 64u + (instancesCount % 64u != 0u);
	auto instancesCountData = uvec4{instancesCountGroups, 1, 1, instancesCount};
	result.instancesCount = Buffer<uvec4>::make(_pool, nameAppend(_name, "instanceCount"),
		vuk::BufferUsageFlagBits::eStorageBuffer |
		vuk::BufferUsageFlagBits::eUniformBuffer |
		vuk::BufferUsageFlagBits::eIndirectBuffer,
		std::span(&instancesCountData, 1));
	result.instances = Buffer<Instance>::make(_pool, nameAppend(_name, "instances"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		instances, vuk::MemoryUsage::eGPUonly, MaxInstances);
	result.colors = Buffer<vec4>::make(_pool, nameAppend(_name, "colors"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		colors, vuk::MemoryUsage::eGPUonly, MaxInstances);
	result.basicTransforms = Buffer<BasicTransform>::make(_pool, nameAppend(_name, "basicTransforms"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		basicTransforms, vuk::MemoryUsage::eGPUonly, MaxInstances);
	_pool.ptc().dma_task();
	
	result.instancesCount.attach(_rg, vuk::eHostWrite, vuk::eNone);
	result.instances.attach(_rg, vuk::eTransferDst, vuk::eNone);
	result.colors.attach(_rg, vuk::eTransferDst, vuk::eNone);
	result.basicTransforms.attach(_rg, vuk::eTransferDst, vuk::eNone);
	
	return result;
	
}

void InstanceList::compile(vuk::PerThreadContext& _ptc) {
	
	auto transformConvPci = vuk::ComputePipelineBaseCreateInfo();
	transformConvPci.add_spirv(std::vector<u32>{
#include "spv/transformConv.comp.spv"
	}, "transformConv.comp");
	_ptc.ctx.create_named_pipeline("transform_conv", transformConvPci);
	
}

auto InstanceList::fromBasic(Pool& _pool, vuk::RenderGraph& _rg, vuk::Name _name,
	BasicInstanceList&& _basic) -> InstanceList {
	
	auto transforms = Buffer<Transform>::make(_pool, nameAppend(_name, "transforms"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		_basic.capacity());
	
	transforms.attach(_rg, vuk::eNone, vuk::eNone);
	
	_rg.add_pass({
		.name = nameAppend(_basic.basicTransforms.name, "conversion"),
		.resources = {
			_basic.instancesCount.resource(vuk::eIndirectRead),
			_basic.basicTransforms.resource(vuk::eComputeRead),
			transforms.resource(vuk::eComputeWrite) },
		.execute = [_basic, transforms](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _basic.instancesCount)
			   .bind_storage_buffer(0, 1, _basic.basicTransforms)
			   .bind_storage_buffer(0, 2, transforms)
			   .bind_compute_pipeline("transform_conv");
			
			cmd.dispatch_indirect(_basic.instancesCount);
			
		}});
	
	return InstanceList{
		.instancesCount = _basic.instancesCount,
		.instances = _basic.instances,
		.colors = _basic.colors,
		.transforms = transforms };
	
}

void DrawableInstanceList::compile(vuk::PerThreadContext& _ptc) {
	
	auto instanceSortCountPci = vuk::ComputePipelineBaseCreateInfo();
	instanceSortCountPci.add_spirv(std::vector<u32>{
#include "spv/instanceSortCount.comp.spv"
	}, "instanceSortCount.comp");
	_ptc.ctx.create_named_pipeline("instance_sort_count", instanceSortCountPci);
	
	auto instanceSortScanPci = vuk::ComputePipelineBaseCreateInfo();
	instanceSortScanPci.add_spirv(std::vector<u32>{
#include "spv/instanceSortScan.comp.spv"
	}, "instanceSortScan.comp");
	_ptc.ctx.create_named_pipeline("instance_sort_scan", instanceSortScanPci);
	
	auto instanceSortWritePci = vuk::ComputePipelineBaseCreateInfo();
	instanceSortWritePci.add_spirv(std::vector<u32>{
#include "spv/instanceSortWrite.comp.spv"
	}, "instanceSortWrite.comp");
	_ptc.ctx.create_named_pipeline("instance_sort_write", instanceSortWritePci);
	
	auto frustumCullPci = vuk::ComputePipelineBaseCreateInfo();
	frustumCullPci.add_spirv(std::vector<u32>{
#include "spv/frustumCull.comp.spv"
	}, "frustumCull.comp");
	_ptc.ctx.create_named_pipeline("frustum_cull", frustumCullPci);
	
}

auto DrawableInstanceList::fromUnsorted(Pool& _pool, vuk::RenderGraph& _rg, vuk::Name _name,
	InstanceList const& _unsorted, ModelBuffer const& _models) -> DrawableInstanceList {
	
	auto result = DrawableInstanceList();
	
	// Create a mostly prefilled command buffer
	
	auto commandsData = pvector<Command>();
	commandsData.reserve(_models.meshes.size());
	
	for (auto& mesh: _models.cpu_meshes) {
		
		commandsData.emplace_back(Command{
			.indexCount = mesh.indexCount,
			.instanceCount = 0, // Calculated in step 1
			.firstIndex = mesh.indexOffset,
			.vertexOffset = 0, // Pre-applied
			.firstInstance = 0 }); // Calculated in step 2
		
	}
	
	result.commands = Buffer<Command>::make(_pool, nameAppend(_name, "commands"),
		vuk::BufferUsageFlagBits::eStorageBuffer | vuk::BufferUsageFlagBits::eIndirectBuffer,
		commandsData, vuk::MemoryUsage::eGPUonly);
	_pool.ptc().dma_task();
	result.commands.attach(_rg, vuk::eTransferDst, vuk::eNone);
	
	// Step 1: Count how many instances there are of each mesh
	
	_rg.add_pass({
		.name = nameAppend(_name, "sort count"),
		.resources = {
			_unsorted.instancesCount.resource(vuk::eIndirectRead),
			_unsorted.instances.resource(vuk::eComputeRead),
			result.commands.resource(vuk::eComputeWrite) },
		.execute = [&_unsorted, result](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _unsorted.instancesCount)
			   .bind_storage_buffer(0, 1, _unsorted.instances)
			   .bind_storage_buffer(0, 2, result.commands)
			   .bind_compute_pipeline("instance_sort_count");
			
			cmd.dispatch_indirect(_unsorted.instancesCount);
			
		}});
	
	// Step 2: Prefix sum the command offset
	
	assert(result.commands.length() <= 1024);
	
	_rg.add_pass({
		.name = nameAppend(_name, "sort scan"),
		.resources = {
			result.commands.resource(vuk::eComputeRW) },
		.execute = [&_models, result](vuk::CommandBuffer& cmd) {
			
			cmd.bind_storage_buffer(0, 0, result.commands)
			   .bind_compute_pipeline("instance_sort_scan");
			
			cmd.specialization_constants(0, vuk::ShaderStageFlagBits::eCompute, u32(_models.meshes.size()));
			
			cmd.dispatch(1);
			
		}});
	
	// Step 3: Write out at sorted position
	
	result.instancesCount = _unsorted.instancesCount;
	result.colors = _unsorted.colors;
	result.transforms = _unsorted.transforms;
	
	result.instances = Buffer<Instance>::make(_pool, nameAppend(_name, "instances"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		_unsorted.capacity());
	result.instances.attach(_rg, vuk::eNone, vuk::eNone);
	
	_rg.add_pass({
		.name = nameAppend(_name, "sort write"),
		.resources = {
			_unsorted.instancesCount.resource(vuk::eIndirectRead),
			_unsorted.instances.resource(vuk::eComputeRead),
			result.commands.resource(vuk::eComputeRW),
			result.instances.resource(vuk::eComputeWrite) },
		.execute = [&_unsorted, result](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _unsorted.instancesCount)
			   .bind_storage_buffer(0, 1, _unsorted.instances)
			   .bind_storage_buffer(0, 2, result.commands)
			   .bind_storage_buffer(0, 3, result.instances)
			   .bind_compute_pipeline("instance_sort_write");
			
			cmd.dispatch_indirect(_unsorted.instancesCount);
			
		}});
	
	return result;
	
}

auto DrawableInstanceList::frustumCull(Pool& _pool, vuk::RenderGraph& _rg, vuk::Name _name,
	DrawableInstanceList const& _source, ModelBuffer const& _models, mat4 _view, mat4 _projection) -> DrawableInstanceList {
	
	auto result = DrawableInstanceList();
	
	// Create a zero-filled command buffer
	
	auto commandsData = ivector<Command>(_source.commands.length());
	result.commands = Buffer<Command>::make(_pool, nameAppend(_name, "commands"),
		vuk::BufferUsageFlagBits::eStorageBuffer | vuk::BufferUsageFlagBits::eIndirectBuffer,
		commandsData, vuk::MemoryUsage::eGPUonly);
	_pool.ptc().dma_task();
	result.commands.attach(_rg, vuk::eHostWrite, vuk::eNone);
	
	// Create destination buffers
	
	auto instancesCountData = uvec4{0, 1, 1, 0}; // Zero-initializing instancesCount
	result.instancesCount = Buffer<uvec4>::make(_pool, nameAppend(_name, "instanceCount"),
		vuk::BufferUsageFlagBits::eStorageBuffer |
		vuk::BufferUsageFlagBits::eUniformBuffer |
		vuk::BufferUsageFlagBits::eIndirectBuffer,
		std::span(&instancesCountData, 1));
	result.instances = Buffer<Instance>::make(_pool, nameAppend(_name, "instance"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		_source.capacity());
	result.colors = _source.colors;
	result.transforms = _source.transforms;
	
	result.instancesCount.attach(_rg, vuk::eNone, vuk::eNone);
	result.instances.attach(_rg, vuk::eNone, vuk::eNone);
	
	_rg.add_pass({
		.name = nameAppend(_name, "frustum cull"),
		.resources = {
			_source.commands.resource(vuk::eComputeRead),
			_source.instancesCount.resource(vuk::eIndirectRead),
			_source.instances.resource(vuk::eComputeRead),
			_source.transforms.resource(vuk::eComputeRead),
			result.commands.resource(vuk::eComputeRW),
			result.instancesCount.resource(vuk::eComputeWrite),
			result.instances.resource(vuk::eComputeWrite) },
		.execute = [&_source, result, &_models, _view, _projection](vuk::CommandBuffer& cmd) {
			
			cmd.bind_storage_buffer(0, 0, _source.commands)
			   .bind_uniform_buffer(0, 1, _source.instancesCount)
			   .bind_storage_buffer(0, 2, _source.instances)
			   .bind_storage_buffer(0, 3, result.transforms)
			   .bind_storage_buffer(0, 4, _models.meshes)
			   .bind_storage_buffer(0, 5, result.commands)
			   .bind_storage_buffer(0, 6, result.instancesCount)
			   .bind_storage_buffer(0, 7, result.instances)
			   .bind_compute_pipeline("frustum_cull");
			
			struct PushConstants {
				mat4 view;
				vec4 frustum;
			};
			auto pushConstants = PushConstants{
				.view = _view,
				.frustum = [_projection] {
					
					auto projectionT = transpose(_projection);
					vec4 frustumX = projectionT[3] + projectionT[0];
					vec4 frustumY = projectionT[3] + projectionT[1];
					frustumX /= length(vec3(frustumX));
					frustumY /= length(vec3(frustumY));
					return vec4{frustumX.x(), frustumX.z(), frustumY.y(), frustumY.z()};
					
				}() };
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, pushConstants);
			cmd.specialization_constants(0, vuk::ShaderStageFlagBits::eCompute, u32(_models.meshes.size()));
			
			cmd.dispatch_indirect(_source.instancesCount);
			
		}});
	
	return result;
	
}

}

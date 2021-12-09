#include "gfx/effects/instanceList.hpp"

#include <cassert>
#include "Tracy.hpp"
#include "base/containers/vector.hpp"
#include "base/util.hpp"
#include "gfx/util.hpp"

namespace minote::gfx {

using namespace base;

auto BasicInstanceList::upload(Pool& _pool, Frame& _frame, vuk::Name _name,
	ObjectPool const& _objects) -> BasicInstanceList {
	
	ZoneScoped;
	
	auto instances = pvector<Instance>();
	auto colors = pvector<vec4>();
	auto basicTransforms = pvector<BasicTransform>();
	
	auto instancesCount = 0u;
	
	// Precalculate mesh count
	
	for (auto id: iota(ObjectID(0), _objects.size())) {
		
		auto& metadata = _objects.metadata[id];
		if (!metadata.exists || !metadata.visible)
			continue;
		
		auto modelID = _objects.modelIDs[id];
		instancesCount += _frame.models.cpu_modelMeshes.at(modelID).size();
		
	}
	
	instances.reserve(instancesCount);
	colors.reserve(instancesCount);
	basicTransforms.reserve(instancesCount);
	
	// Queue up meshes of all valid objects
	
	for (auto id: iota(ObjectID(0), _objects.size())) {
		
		auto& metadata = _objects.metadata[id];
		if (!metadata.exists || !metadata.visible)
			continue;
		
		auto modelID = _objects.modelIDs[id];
		for (auto meshIdx: _frame.models.cpu_modelMeshes.at(modelID)) {
			
			instances.emplace_back(Instance{
				.meshIdx = u32(meshIdx),
				.transformIdx = u32(basicTransforms.size()) });
			colors.emplace_back(_objects.colors[id]);
			basicTransforms.emplace_back(_objects.transforms[id]);
			
		}
		
	}
	
	// Upload data to GPU
	
	auto result = BasicInstanceList();
	
	auto instancesCountGroups = divRoundUp(instancesCount, 64u);
	auto instancesCountData = uvec4{instancesCountGroups, 1, 1, instancesCount};
	result.instancesCount = Buffer<uvec4>::make(_pool, nameAppend(_name, "instanceCount"),
		vuk::BufferUsageFlagBits::eStorageBuffer |
		vuk::BufferUsageFlagBits::eUniformBuffer |
		vuk::BufferUsageFlagBits::eIndirectBuffer,
		std::span(&instancesCountData, 1));
	result.instances = Buffer<Instance>::make(_pool, nameAppend(_name, "instances"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		instances, MaxInstances);
	result.colors = Buffer<vec4>::make(_pool, nameAppend(_name, "colors"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		colors, MaxInstances);
	result.basicTransforms = Buffer<BasicTransform>::make(_pool, nameAppend(_name, "basicTransforms"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		basicTransforms, MaxInstances);
	
	result.instancesCount.attach(_frame.rg, vuk::eHostWrite, vuk::eNone);
	result.instances.attach(_frame.rg, vuk::eTransferDst, vuk::eNone);
	result.colors.attach(_frame.rg, vuk::eTransferDst, vuk::eNone);
	result.basicTransforms.attach(_frame.rg, vuk::eTransferDst, vuk::eNone);
	
	return result;
	
}

void InstanceList::compile(vuk::PerThreadContext& _ptc) {
	
	auto transformConvPci = vuk::ComputePipelineBaseCreateInfo();
	transformConvPci.add_spirv(std::vector<u32>{
#include "spv/transformConv.comp.spv"
	}, "transformConv.comp");
	_ptc.ctx.create_named_pipeline("transform_conv", transformConvPci);
	
}

auto InstanceList::fromBasic(Pool& _pool, Frame& _frame, vuk::Name _name,
	BasicInstanceList&& _basic) -> InstanceList {
	
	auto transforms = Buffer<Transform>::make(_pool, nameAppend(_name, "transforms"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		_basic.capacity());
	
	transforms.attach(_frame.rg, vuk::eNone, vuk::eNone);
	
	_frame.rg.add_pass({
		.name = nameAppend(_basic.basicTransforms.name, "conversion"),
		.resources = {
			_basic.instancesCount.resource(vuk::eIndirectRead),
			_basic.basicTransforms.resource(vuk::eComputeRead),
			transforms.resource(vuk::eComputeWrite) },
		.execute = [_basic, transforms, &_frame](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _basic.instancesCount)
			   .bind_storage_buffer(0, 1, _basic.basicTransforms)
			   .bind_storage_buffer(0, 2, transforms)
			   .bind_storage_buffer(0, 3, _basic.instances)
			   .bind_storage_buffer(0, 4, _frame.models.meshes)
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
	
	auto instanceSortScan1Pci = vuk::ComputePipelineBaseCreateInfo();
	instanceSortScan1Pci.add_spirv(std::vector<u32>{
#include "spv/instanceSortScan1.comp.spv"
	}, "instanceSortScan1.comp");
	_ptc.ctx.create_named_pipeline("instance_sort_scan1", instanceSortScan1Pci);
	
	auto instanceSortScan2Pci = vuk::ComputePipelineBaseCreateInfo();
	instanceSortScan2Pci.add_spirv(std::vector<u32>{
#include "spv/instanceSortScan2.comp.spv"
	}, "instanceSortScan2.comp");
	_ptc.ctx.create_named_pipeline("instance_sort_scan2", instanceSortScan2Pci);
	
	auto instanceSortScan3Pci = vuk::ComputePipelineBaseCreateInfo();
	instanceSortScan3Pci.add_spirv(std::vector<u32>{
#include "spv/instanceSortScan3.comp.spv"
	}, "instanceSortScan3.comp");
	_ptc.ctx.create_named_pipeline("instance_sort_scan3", instanceSortScan3Pci);
	
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

auto DrawableInstanceList::fromUnsorted(Pool& _pool, Frame& _frame, vuk::Name _name,
	InstanceList _unsorted) -> DrawableInstanceList {
	
	auto& rg = _frame.rg;
	
	auto result = DrawableInstanceList();
	
	// Create a mostly prefilled command buffer
	
	auto commandsData = pvector<Command>();
	commandsData.reserve(_frame.models.cpu_meshes.size());
	
	for (auto& mesh: _frame.models.cpu_meshes) {
		
		commandsData.emplace_back(Command{
			.indexCount = mesh.indexCount,
			.instanceCount = 0, // Calculated in step 1
			.firstIndex = mesh.indexOffset,
			.vertexOffset = 0, // Pre-applied
			.firstInstance = 0 }); // Calculated in step 2
		
	}
	
	result.commands = Buffer<Command>::make(_pool, nameAppend(_name, "commands"),
		vuk::BufferUsageFlagBits::eStorageBuffer | vuk::BufferUsageFlagBits::eIndirectBuffer,
		commandsData);
	result.commands.attach(rg, vuk::eTransferDst, vuk::eNone);
	
	// Step 1: Count how many instances there are of each mesh
	
	rg.add_pass({
		.name = nameAppend(_name, "sort count"),
		.resources = {
			_unsorted.instancesCount.resource(vuk::eIndirectRead),
			_unsorted.instances.resource(vuk::eComputeRead),
			result.commands.resource(vuk::eComputeWrite) },
		.execute = [_unsorted, result](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _unsorted.instancesCount)
			   .bind_storage_buffer(0, 1, _unsorted.instances)
			   .bind_storage_buffer(0, 2, result.commands)
			   .bind_compute_pipeline("instance_sort_count");
			
			cmd.dispatch_indirect(_unsorted.instancesCount);
			
		}});
	
	// Step 2: Prefix sum the command offset
	
	assert(result.commands.length() <= 1024 * 1024);
	
	auto scanTemp = Buffer<u32>::make(_frame.framePool, nameAppend(_name, "scanTemp"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		divRoundUp(result.commands.length(), 1024_zu));
	scanTemp.attach(rg, vuk::eNone, vuk::eNone);
	
	rg.add_pass({
		.name = nameAppend(_name, "sort scan1"),
		.resources = {
			result.commands.resource(vuk::eComputeRW),
			scanTemp.resource(vuk::eComputeWrite) },
		.execute = [result, scanTemp](vuk::CommandBuffer& cmd) {
			
			cmd.bind_storage_buffer(0, 0, result.commands)
			   .bind_storage_buffer(0, 1, scanTemp)
			   .bind_compute_pipeline("instance_sort_scan1");
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, u32(result.commands.length()));
			
			cmd.dispatch_invocations(result.commands.length());
			
		}});
	
	rg.add_pass({
		.name = nameAppend(_name, "sort scan2"),
		.resources = {
			scanTemp.resource(vuk::eComputeRW) },
		.execute = [scanTemp](vuk::CommandBuffer& cmd) {
			
			cmd.bind_storage_buffer(0, 0, scanTemp)
			   .bind_compute_pipeline("instance_sort_scan2");
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, u32(scanTemp.length()));
			
			cmd.dispatch_invocations(scanTemp.length());
			
	}});
	
	rg.add_pass({
		.name = nameAppend(_name, "sort scan3"),
		.resources = {
			scanTemp.resource(vuk::eComputeRead),
			result.commands.resource(vuk::eComputeRW) },
		.execute = [scanTemp, result](vuk::CommandBuffer& cmd) {
			
			cmd.bind_storage_buffer(0, 0, scanTemp)
			   .bind_storage_buffer(0, 1, result.commands)
			   .bind_compute_pipeline("instance_sort_scan3");
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, u32(result.commands.length()));
			
			cmd.dispatch_invocations(result.commands.length());
			
	}});
	
	// Step 3: Write out at sorted position
	
	result.instancesCount = _unsorted.instancesCount;
	result.colors = _unsorted.colors;
	result.transforms = _unsorted.transforms;
	
	result.instances = Buffer<Instance>::make(_pool, nameAppend(_name, "instances"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		_unsorted.capacity());
	result.instances.attach(rg, vuk::eNone, vuk::eNone);
	
	rg.add_pass({
		.name = nameAppend(_name, "sort write"),
		.resources = {
			_unsorted.instancesCount.resource(vuk::eIndirectRead),
			_unsorted.instances.resource(vuk::eComputeRead),
			result.commands.resource(vuk::eComputeRW),
			result.instances.resource(vuk::eComputeWrite) },
		.execute = [_unsorted, result](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _unsorted.instancesCount)
			   .bind_storage_buffer(0, 1, _unsorted.instances)
			   .bind_storage_buffer(0, 2, result.commands)
			   .bind_storage_buffer(0, 3, result.instances)
			   .bind_compute_pipeline("instance_sort_write");
			
			cmd.dispatch_indirect(_unsorted.instancesCount);
			
		}});
	
	return result;
	
}

auto DrawableInstanceList::frustumCull(Pool& _pool, Frame& _frame, vuk::Name _name,
	DrawableInstanceList _source, mat4 _view, mat4 _projection) -> DrawableInstanceList {
	
	auto result = DrawableInstanceList();
	
	// Create a zero-filled command buffer
	
	auto commandsData = ivector<Command>(_source.commands.length());
	result.commands = Buffer<Command>::make(_pool, nameAppend(_name, "commands"),
		vuk::BufferUsageFlagBits::eStorageBuffer | vuk::BufferUsageFlagBits::eIndirectBuffer,
		commandsData);
	result.commands.attach(_frame.rg, vuk::eHostWrite, vuk::eNone);
	
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
	
	result.instancesCount.attach(_frame.rg, vuk::eNone, vuk::eNone);
	result.instances.attach(_frame.rg, vuk::eNone, vuk::eNone);
	
	_frame.rg.add_pass({
		.name = nameAppend(_name, "frustum cull"),
		.resources = {
			_source.commands.resource(vuk::eComputeRead),
			_source.instancesCount.resource(vuk::eIndirectRead),
			_source.instances.resource(vuk::eComputeRead),
			_source.transforms.resource(vuk::eComputeRead),
			result.commands.resource(vuk::eComputeRW),
			result.instancesCount.resource(vuk::eComputeWrite),
			result.instances.resource(vuk::eComputeWrite) },
		.execute = [_source, result, &_frame, _view, _projection](vuk::CommandBuffer& cmd) {
			
			cmd.bind_storage_buffer(0, 0, _source.commands)
			   .bind_uniform_buffer(0, 1, _source.instancesCount)
			   .bind_storage_buffer(0, 2, _source.instances)
			   .bind_storage_buffer(0, 3, result.transforms)
			   .bind_storage_buffer(0, 4, _frame.models.meshes)
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
			cmd.specialization_constants(0, vuk::ShaderStageFlagBits::eCompute, u32(_frame.models.cpu_meshes.size()));
			
			cmd.dispatch_indirect(_source.instancesCount);
			
		}});
	
	return result;
	
}

}

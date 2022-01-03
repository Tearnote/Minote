#include "gfx/effects/instanceList.hpp"

#include <cassert>
#include "Tracy.hpp"
#include "base/containers/vector.hpp"
#include "base/util.hpp"
#include "gfx/util.hpp"

namespace minote::gfx {

using namespace base;

auto BasicObjectList::upload(Pool& _pool, Frame& _frame, vuk::Name _name,
	ObjectPool const& _objects) -> BasicObjectList {
	
	ZoneScoped;
	
	auto modelIndices = pvector<u32>();
	auto colors = pvector<vec4>();
	auto basicTransforms = pvector<BasicTransform>();
	
	auto objectsCount = 0u;
	
	// Precalculate object count
	
	for (auto id: iota(ObjectID(0), _objects.size())) {
		
		auto& metadata = _objects.metadata[id];
		if (!metadata.exists || !metadata.visible)
			continue;
		
		objectsCount += 1;
		
	}
	
	modelIndices.reserve(objectsCount);
	colors.reserve(objectsCount);
	basicTransforms.reserve(objectsCount);
	
	// Queue up all valid objects
	
	for (auto id: iota(ObjectID(0), _objects.size())) {
		
		auto& metadata = _objects.metadata[id];
		if (!metadata.exists || !metadata.visible)
			continue;
		
		auto modelID = _objects.modelIDs[id];
		auto modelIdx = _frame.models.cpu_modelIndices.at(modelID);
		modelIndices.emplace_back(modelIdx);
		colors.emplace_back(_objects.colors[id]);
		basicTransforms.emplace_back(_objects.transforms[id]);
		
	}
	
	// Upload data to GPU
	
	auto result = BasicObjectList();
	
	auto objectsCountGroups = divRoundUp(objectsCount, 1024u);
	auto objectsCountData = uvec4{objectsCountGroups, 1, 1, objectsCount};
	result.objectsCount = Buffer<uvec4>::make(_pool, nameAppend(_name, "objectsCount"),
		vuk::BufferUsageFlagBits::eStorageBuffer |
		vuk::BufferUsageFlagBits::eUniformBuffer |
		vuk::BufferUsageFlagBits::eIndirectBuffer,
		std::span(&objectsCountData, 1));
	result.modelIndices = Buffer<u32>::make(_pool, nameAppend(_name, "modelIndices"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		modelIndices, MaxObjects);
	result.colors = Buffer<vec4>::make(_pool, nameAppend(_name, "colors"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		colors, MaxObjects);
	result.basicTransforms = Buffer<BasicTransform>::make(_pool, nameAppend(_name, "basicTransforms"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		basicTransforms, MaxObjects);
	
	result.objectsCount.attach(_frame.rg, vuk::eHostWrite, vuk::eNone);
	result.modelIndices.attach(_frame.rg, vuk::eHostWrite, vuk::eNone);
	result.colors.attach(_frame.rg, vuk::eHostWrite, vuk::eNone);
	result.basicTransforms.attach(_frame.rg, vuk::eHostWrite, vuk::eNone);
	
	return result;
	
}

void ObjectList::compile(vuk::PerThreadContext& _ptc) {
	
	auto transformConvPci = vuk::ComputePipelineBaseCreateInfo();
	transformConvPci.add_spirv(std::vector<u32>{
#include "spv/instanceList/transformConv.comp.spv"
	}, "instanceList/transformConv.comp");
	_ptc.ctx.create_named_pipeline("objectList/transformConv", transformConvPci);
	
}

auto ObjectList::fromBasic(Pool& _pool, Frame& _frame, vuk::Name _name,
	BasicObjectList&& _basic) -> ObjectList {
	
	auto transforms = Buffer<Transform>::make(_pool, nameAppend(_name, "transforms"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		_basic.capacity());
	
	transforms.attach(_frame.rg, vuk::eNone, vuk::eNone);
	
	_frame.rg.add_pass({
		.name = nameAppend(_basic.basicTransforms.name, "objectList/transformConv"),
		.resources = {
			_basic.objectsCount.resource(vuk::eIndirectRead),
			_basic.basicTransforms.resource(vuk::eComputeRead),
			transforms.resource(vuk::eComputeWrite) },
		.execute = [_basic, transforms, &_frame](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _basic.objectsCount)
			   .bind_storage_buffer(0, 1, _basic.basicTransforms)
			   .bind_storage_buffer(0, 2, transforms)
			   .bind_compute_pipeline("objectList/transformConv");
			
			cmd.dispatch_indirect(_basic.objectsCount);
			
		}});
	
	return ObjectList{
		.objectsCount = _basic.objectsCount,
		.modelIndices = _basic.modelIndices,
		.colors = _basic.colors,
		.transforms = transforms };
	
}

void InstanceList::compile(vuk::PerThreadContext& _ptc) {
	
	auto objectScan1Pci = vuk::ComputePipelineBaseCreateInfo();
	objectScan1Pci.add_spirv(std::vector<u32>{
#include "spv/instanceList/objectScan1.comp.spv"
	}, "instanceList/objectScan1.comp");
	_ptc.ctx.create_named_pipeline("instanceList/objectScan1", objectScan1Pci);
	
	auto objectScan2Pci = vuk::ComputePipelineBaseCreateInfo();
	objectScan2Pci.add_spirv(std::vector<u32>{
#include "spv/instanceList/objectScan2.comp.spv"
	}, "instanceList/objectScan2.comp");
	_ptc.ctx.create_named_pipeline("instanceList/objectScan2", objectScan2Pci);
	
	auto objectScan3Pci = vuk::ComputePipelineBaseCreateInfo();
	objectScan3Pci.add_spirv(std::vector<u32>{
#include "spv/instanceList/objectScan3.comp.spv"
	}, "instanceList/objectScan3.comp");
	_ptc.ctx.create_named_pipeline("instanceList/objectScan3", objectScan3Pci);
	
	auto instanceScan1Pci = vuk::ComputePipelineBaseCreateInfo();
	instanceScan1Pci.add_spirv(std::vector<u32>{
#include "spv/instanceList/instanceScan1.comp.spv"
	}, "instanceList/instanceScan1.comp");
	_ptc.ctx.create_named_pipeline("instanceList/instanceScan1", instanceScan1Pci);
	
	auto instanceScan2Pci = vuk::ComputePipelineBaseCreateInfo();
	instanceScan2Pci.add_spirv(std::vector<u32>{
#include "spv/instanceList/instanceScan2.comp.spv"
	}, "instanceList/instanceScan2.comp");
	_ptc.ctx.create_named_pipeline("instanceList/instanceScan2", instanceScan2Pci);
	
	auto instanceScan3Pci = vuk::ComputePipelineBaseCreateInfo();
	instanceScan3Pci.add_spirv(std::vector<u32>{
#include "spv/instanceList/instanceScan3.comp.spv"
	}, "instanceList/instanceScan3.comp");
	_ptc.ctx.create_named_pipeline("instanceList/instanceScan3", instanceScan3Pci);
	
	auto instanceSortScan1Pci = vuk::ComputePipelineBaseCreateInfo();
	instanceSortScan1Pci.add_spirv(std::vector<u32>{
#include "spv/instanceList/sortScan1.comp.spv"
	}, "instanceList/sortScan1.comp");
	_ptc.ctx.create_named_pipeline("instanceList/sortScan1", instanceSortScan1Pci);
	
	auto instanceSortScan2Pci = vuk::ComputePipelineBaseCreateInfo();
	instanceSortScan2Pci.add_spirv(std::vector<u32>{
#include "spv/instanceList/sortScan2.comp.spv"
	}, "instanceList/sortScan2.comp");
	_ptc.ctx.create_named_pipeline("instanceList/sortScan2", instanceSortScan2Pci);
	
	auto instanceSortScan3Pci = vuk::ComputePipelineBaseCreateInfo();
	instanceSortScan3Pci.add_spirv(std::vector<u32>{
#include "spv/instanceList/sortScan3.comp.spv"
	}, "instanceList/sortScan3.comp");
	_ptc.ctx.create_named_pipeline("instanceList/sortScan3", instanceSortScan3Pci);
	
	auto instanceSortWritePci = vuk::ComputePipelineBaseCreateInfo();
	instanceSortWritePci.add_spirv(std::vector<u32>{
#include "spv/instanceList/sortWrite.comp.spv"
	}, "instanceList/sortWrite.comp");
	_ptc.ctx.create_named_pipeline("instanceList/sortWrite", instanceSortWritePci);
	
}

auto InstanceList::fromObjects(Pool& _pool, Frame& _frame, vuk::Name _name,
	ObjectList _objects, mat4 _view, mat4 _projection) -> InstanceList {
	
	auto& rg = _frame.rg;
	
	auto result = InstanceList();
	
	// Prefix sum over object mesh count
	
	assert(_objects.capacity() <= 1024 * 1024);
	
	auto objectsScan = Buffer<u32>::make(_frame.permPool, nameAppend(_name, "objectsScan"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		_objects.capacity());
	auto objectsScanTemp = Buffer<u32>::make(_frame.permPool, nameAppend(_name, "objectsScanTemp"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		divRoundUp(_objects.capacity(), 1024_zu));
	objectsScan.attach(rg, vuk::eNone, vuk::eNone);
	objectsScanTemp.attach(rg, vuk::eNone, vuk::eNone);
	
	auto instancesTempInit = ivector<u32>(InstanceList::capacity());
	auto instancesTemp = Buffer<u32>::make(_frame.framePool, nameAppend(_name, "instancesTemp"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		instancesTempInit);
	instancesTemp.attach(rg, vuk::eNone, vuk::eNone);
	
	result.instancesCount = Buffer<uvec4>::make(_pool, nameAppend(_name, "instancesCount"),
		vuk::BufferUsageFlagBits::eIndirectBuffer |
		vuk::BufferUsageFlagBits::eStorageBuffer |
		vuk::BufferUsageFlagBits::eUniformBuffer);
	result.instancesCount.attach(rg, vuk::eNone, vuk::eNone);
	
	rg.add_pass({
		.name = nameAppend(_name, "instanceList/objectScan1"),
		.resources = {
			_objects.objectsCount.resource(vuk::eIndirectRead),
			_objects.modelIndices.resource(vuk::eComputeRead),
			objectsScanTemp.resource(vuk::eComputeWrite),
			objectsScan.resource(vuk::eComputeWrite) },
		.execute = [&_frame, _objects, objectsScan, objectsScanTemp](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _objects.objectsCount)
			   .bind_storage_buffer(0, 1, _frame.models.models)
			   .bind_storage_buffer(0, 2, _objects.modelIndices)
			   .bind_storage_buffer(0, 3, objectsScanTemp)
			   .bind_storage_buffer(0, 4, objectsScan)
			   .bind_compute_pipeline("instanceList/objectScan1");
			
			cmd.dispatch_indirect(_objects.objectsCount);
			
		}});
	
	rg.add_pass({
		.name = nameAppend(_name, "instanceList/objectScan2"),
		.resources = {
			_objects.objectsCount.resource(vuk::eComputeRead),
			objectsScanTemp.resource(vuk::eComputeRW) },
		.execute = [_objects, objectsScanTemp](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _objects.objectsCount)
			   .bind_storage_buffer(0, 1, objectsScanTemp)
			   .bind_compute_pipeline("instanceList/objectScan2");
			
			cmd.dispatch(1);
			
	}});
	
	rg.add_pass({
		.name = nameAppend(_name, "instanceList/objectScan3"),
		.resources = {
			_objects.objectsCount.resource(vuk::eIndirectRead),
			_objects.modelIndices.resource(vuk::eComputeRead),
			objectsScanTemp.resource(vuk::eComputeRead),
			objectsScan.resource(vuk::eComputeRW),
			instancesTemp.resource(vuk::eComputeWrite), // Span starts are marked here in preparation for next step
			result.instancesCount.resource(vuk::eComputeWrite) }, // Last invocation writes instance count
		.execute = [&_frame, result, _objects, objectsScanTemp, objectsScan, instancesTemp](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _objects.objectsCount)
			   .bind_storage_buffer(0, 1, _frame.models.models)
			   .bind_storage_buffer(0, 2, _objects.modelIndices)
			   .bind_storage_buffer(0, 3, objectsScanTemp)
			   .bind_storage_buffer(0, 4, objectsScan)
			   .bind_storage_buffer(0, 5, instancesTemp)
			   .bind_storage_buffer(0, 6, result.instancesCount)
			   .bind_compute_pipeline("instanceList/objectScan3");
			
			cmd.dispatch_indirect(_objects.objectsCount);
			
		}});
	
	// Prefix sum over instance list
	
	auto instancesScanTemp = Buffer<u32>::make(_frame.permPool, nameAppend(_name, "instancesScanTemp"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		divRoundUp(capacity(), 1024_zu));
	instancesScanTemp.attach(rg, vuk::eNone, vuk::eNone);
	
	auto instancesUnsorted = Buffer<Instance>::make(_frame.permPool, nameAppend(_name, "instancesUnsorted"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		InstanceList::capacity());
	instancesUnsorted.attach(rg, vuk::eNone, vuk::eNone);
	
	rg.add_pass({
		.name = nameAppend(_name, "instanceList/instanceScan1"),
		.resources = {
			result.instancesCount.resource(vuk::eIndirectRead),
			instancesTemp.resource(vuk::eComputeRW),
			instancesScanTemp.resource(vuk::eComputeWrite) },
		.execute = [result, instancesTemp, instancesScanTemp](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, result.instancesCount)
			   .bind_storage_buffer(0, 1, instancesTemp)
			   .bind_storage_buffer(0, 2, instancesScanTemp)
			   .bind_compute_pipeline("instanceList/instanceScan1");
			
			cmd.dispatch_indirect(result.instancesCount);
			
		}});
	
	rg.add_pass({
		.name = nameAppend(_name, "instanceList/instanceScan2"),
		.resources = {
			result.instancesCount.resource(vuk::eComputeRead),
			instancesScanTemp.resource(vuk::eComputeRW) },
		.execute = [result, instancesScanTemp](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, result.instancesCount)
			   .bind_storage_buffer(0, 1, instancesScanTemp)
			   .bind_compute_pipeline("instanceList/instanceScan2");
			
			cmd.dispatch(1);
			
	}});
	
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
	result.commands = Buffer<Command>::make(_frame.framePool, nameAppend(_name, "commands"),
		vuk::BufferUsageFlagBits::eStorageBuffer |
		vuk::BufferUsageFlagBits::eIndirectBuffer,
		commandsData);
	result.commands.attach(rg, vuk::eHostWrite, vuk::eNone);
	
	rg.add_pass({
		.name = nameAppend(_name, "instanceList/instanceScan3"),
		.resources = {
			result.instancesCount.resource(vuk::eIndirectRead),
			_objects.modelIndices.resource(vuk::eComputeRead),
			_objects.transforms.resource(vuk::eComputeRead),
			objectsScan.resource(vuk::eComputeRead),
			instancesScanTemp.resource(vuk::eComputeRead),
			instancesTemp.resource(vuk::eComputeRead),
			instancesUnsorted.resource(vuk::eComputeWrite),
			result.commands.resource(vuk::eComputeRW) },
		.execute = [&_frame, _objects, result, objectsScan, instancesScanTemp,
			instancesTemp, instancesUnsorted, _view, _projection](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, result.instancesCount)
			   .bind_storage_buffer(0, 1, _frame.models.models)
			   .bind_storage_buffer(0, 2, _frame.models.meshes)
			   .bind_storage_buffer(0, 3, _objects.modelIndices)
			   .bind_storage_buffer(0, 4, _objects.transforms)
			   .bind_storage_buffer(0, 5, objectsScan)
			   .bind_storage_buffer(0, 6, instancesScanTemp)
			   .bind_storage_buffer(0, 7, instancesTemp)
			   .bind_storage_buffer(0, 8, instancesUnsorted)
			   .bind_storage_buffer(0, 9, result.commands)
			   .bind_compute_pipeline("instanceList/instanceScan3");
			
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
			
			cmd.dispatch_indirect(result.instancesCount);
			
		}});
	
	// Prefix sum the command offset
	
	assert(result.commands.length() <= 1024 * 1024);
	
	auto scanTemp = Buffer<u32>::make(_frame.framePool, nameAppend(_name, "scanTemp"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		divRoundUp(result.commands.length(), 1024_zu));
	scanTemp.attach(rg, vuk::eNone, vuk::eNone);
	
	rg.add_pass({
		.name = nameAppend(_name, "instanceList/sortScan1"),
		.resources = {
			result.commands.resource(vuk::eComputeRW),
			scanTemp.resource(vuk::eComputeWrite) },
		.execute = [result, scanTemp](vuk::CommandBuffer& cmd) {
			
			cmd.bind_storage_buffer(0, 0, result.commands)
			   .bind_storage_buffer(0, 1, scanTemp)
			   .bind_compute_pipeline("instanceList/sortScan1");
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, u32(result.commands.length()));
			
			cmd.dispatch_invocations(result.commands.length());
			
		}});
	
	rg.add_pass({
		.name = nameAppend(_name, "instanceList/sortScan2"),
		.resources = {
			scanTemp.resource(vuk::eComputeRW) },
		.execute = [scanTemp](vuk::CommandBuffer& cmd) {
			
			cmd.bind_storage_buffer(0, 0, scanTemp)
			   .bind_compute_pipeline("instanceList/sortScan2");
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, u32(scanTemp.length()));
			
			cmd.dispatch_invocations(1);
			
		}});
	
	rg.add_pass({
		.name = nameAppend(_name, "instanceList/sortScan3"),
		.resources = {
			scanTemp.resource(vuk::eComputeRead),
			result.commands.resource(vuk::eComputeRW) },
		.execute = [scanTemp, result](vuk::CommandBuffer& cmd) {
			
			cmd.bind_storage_buffer(0, 0, scanTemp)
			   .bind_storage_buffer(0, 1, result.commands)
			   .bind_compute_pipeline("instanceList/sortScan3");
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, u32(result.commands.length()));
			
			cmd.dispatch_invocations(result.commands.length());
			
		}});
	
	// Write out at sorted position
	
	result.colors = _objects.colors;
	result.transforms = _objects.transforms;
	
	result.instances = Buffer<Instance>::make(_pool, nameAppend(_name, "instances"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		capacity());
	result.instances.attach(rg, vuk::eNone, vuk::eNone);
	
	rg.add_pass({
		.name = nameAppend(_name, "instanceList/sortWrite"),
		.resources = {
			result.instancesCount.resource(vuk::eIndirectRead),
			instancesUnsorted.resource(vuk::eComputeRead),
			result.commands.resource(vuk::eComputeRW),
			result.instances.resource(vuk::eComputeWrite) },
		.execute = [result, instancesUnsorted](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, result.instancesCount)
			   .bind_storage_buffer(0, 1, instancesUnsorted)
			   .bind_storage_buffer(0, 2, result.commands)
			   .bind_storage_buffer(0, 3, result.instances)
			   .bind_compute_pipeline("instanceList/sortWrite");
			
			cmd.dispatch_indirect(result.instancesCount);
			
		}});
	
	return result;
	
}

}

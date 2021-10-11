#include "gfx/effects/instanceList.hpp"

#include <cassert>
#include "Tracy.hpp"
#include "base/containers/vector.hpp"
#include "base/util.hpp"
#include "gfx/util.hpp"

namespace minote::gfx {

using namespace base;

auto BasicInstanceList::upload(Pool& _pool, vuk::RenderGraph& _rg,
	vuk::Name _name, ObjectPool const& _objects,
	MeshBuffer const& _meshes, MaterialBuffer const& _materials) -> BasicInstanceList {
	
	ZoneScoped;
	
	auto instances = pvector<Instance>(_objects.size());
	auto colors = pvector<vec4>(_objects.size());
	auto basicTransforms = pvector<BasicTransform>(_objects.size());
	
	auto instancesCount = 0u;
	
	// Copy all valid objects
	
	for (auto id: iota(ObjectID(0), _objects.size())) {
		
		auto& metadata = _objects.metadata[id];
		if (!metadata.exists || !metadata.visible)
			continue;
		
		auto meshID = _objects.meshIDs[id];
		auto meshIdx = _meshes.descriptorIDs.at(meshID);
		auto materialID = _objects.materialIDs[id];
		auto materialIdx = _materials.materialIDs.at(materialID);
		
		instances[instancesCount] = Instance{
			.meshIdx = u32(meshIdx),
			.materialIdx = u32(materialIdx),
			.colorIdx = instancesCount,
			.transformIdx = instancesCount };
		colors[instancesCount] = _objects.colors[id];
		basicTransforms[instancesCount] = _objects.transforms[id];
		
		instancesCount += 1;
		
	}
	
	instances.resize(instancesCount);
	colors.resize(instancesCount);
	basicTransforms.resize(instancesCount);
	
	// Upload data to GPU
	
	auto result = BasicInstanceList();
	
	result.instancesCount = Buffer<u32>::make(_pool, nameAppend(_name, "instanceCount"),
		vuk::BufferUsageFlagBits::eStorageBuffer | vuk::BufferUsageFlagBits::eUniformBuffer,
		std::span(&instancesCount, 1));
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
	
	auto transformConvPci = vuk::ComputePipelineCreateInfo();
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
			_basic.instancesCount.resource(vuk::eComputeRead),
			_basic.basicTransforms.resource(vuk::eComputeRead),
			transforms.resource(vuk::eComputeWrite) },
		.execute = [_basic, transforms](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _basic.instancesCount)
			   .bind_storage_buffer(0, 1, _basic.basicTransforms)
			   .bind_storage_buffer(0, 2, transforms)
			   .bind_compute_pipeline("transform_conv");
			
			struct PushConstants {
				mat4 view;
				vec4 frustum;
				u32 instancesCount;
			};
			
			cmd.dispatch_invocations(_basic.capacity());
			
		}});
	
	return InstanceList{
		.instancesCount = _basic.instancesCount,
		.instances = _basic.instances,
		.colors = _basic.colors,
		.transforms = transforms };
	
}

void DrawableInstanceList::compile(vuk::PerThreadContext& _ptc) {
	
	auto instanceSortCountPci = vuk::ComputePipelineCreateInfo();
	instanceSortCountPci.add_spirv(std::vector<u32>{
#include "spv/instanceSortCount.comp.spv"
	}, "instanceSortCount.comp");
	_ptc.ctx.create_named_pipeline("instance_sort_count", instanceSortCountPci);
	
	auto instanceSortScanPci = vuk::ComputePipelineCreateInfo();
	instanceSortScanPci.add_spirv(std::vector<u32>{
#include "spv/instanceSortScan.comp.spv"
	}, "instanceSortScan.comp");
	_ptc.ctx.create_named_pipeline("instance_sort_scan", instanceSortScanPci);
	
	auto instanceSortWritePci = vuk::ComputePipelineCreateInfo();
	instanceSortWritePci.add_spirv(std::vector<u32>{
#include "spv/instanceSortWrite.comp.spv"
	}, "instanceSortWrite.comp");
	_ptc.ctx.create_named_pipeline("instance_sort_write", instanceSortWritePci);
	
	auto frustumCullPci = vuk::ComputePipelineCreateInfo();
	frustumCullPci.add_spirv(std::vector<u32>{
#include "spv/frustumCull.comp.spv"
	}, "frustumCull.comp");
	_ptc.ctx.create_named_pipeline("frustum_cull", frustumCullPci);
	
}

auto DrawableInstanceList::fromUnsorted(Pool& _pool, vuk::RenderGraph& _rg, vuk::Name _name,
	InstanceList const& _unsorted, MeshBuffer const& _meshes) -> DrawableInstanceList {
	
	auto result = DrawableInstanceList();
	
	// Create a mostly prefilled command buffer
	
	auto commandsData = pvector<Command>();
	commandsData.reserve(_meshes.descriptors.size());
	
	for (auto& descriptor: _meshes.descriptors) {
		
		commandsData.emplace_back(Command{
			.indexCount = descriptor.indexCount,
			.instanceCount = 0, // Calculated in step 1
			.firstIndex = descriptor.indexOffset,
			.vertexOffset = i32(descriptor.vertexOffset),
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
			_unsorted.instancesCount.resource(vuk::eComputeRead),
			_unsorted.instances.resource(vuk::eComputeRead),
			result.commands.resource(vuk::eComputeWrite) },
		.execute = [&_unsorted, result](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _unsorted.instancesCount)
			   .bind_storage_buffer(0, 1, _unsorted.instances)
			   .bind_storage_buffer(0, 2, result.commands)
			   .bind_compute_pipeline("instance_sort_count");
			
			cmd.dispatch_invocations(_unsorted.capacity());
			
		}});
	
	// Step 2: Prefix sum the command offset
	
	assert(result.commands.length() <= 1024);
	
	_rg.add_pass({
		.name = nameAppend(_name, "sort scan"),
		.resources = {
			result.commands.resource(vuk::eComputeRW) },
		.execute = [&_meshes, result](vuk::CommandBuffer& cmd) {
			
			cmd.bind_storage_buffer(0, 0, result.commands)
			   .push_constants(vuk::ShaderStageFlagBits::eCompute, 0, u32(_meshes.descriptors.size()))
			   .bind_compute_pipeline("instance_sort_scan");
			
			cmd.dispatch(1);
			
		}});
	
	// Step 3: Write out at sorted position
	
	result.instancesCount = Buffer<u32>::make(_pool, nameAppend(_name, "instanceCount"),
		vuk::BufferUsageFlagBits::eStorageBuffer | vuk::BufferUsageFlagBits::eUniformBuffer);
	result.instances = Buffer<Instance>::make(_pool, nameAppend(_name, "instances"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		_unsorted.capacity());
	result.colors = _unsorted.colors;
	result.transforms = _unsorted.transforms;
	
	result.instancesCount.attach(_rg, vuk::eNone, vuk::eNone);
	result.instances.attach(_rg, vuk::eNone, vuk::eNone);
	
	_rg.add_pass({
		.name = nameAppend(_name, "sort write"),
		.resources = {
			_unsorted.instancesCount.resource(vuk::eComputeRead),
			_unsorted.instances.resource(vuk::eComputeRead),
			result.commands.resource(vuk::eComputeRW),
			result.instancesCount.resource(vuk::eComputeWrite),
			result.instances.resource(vuk::eComputeWrite) },
		.execute = [&_unsorted, result](vuk::CommandBuffer& cmd) {
			
			cmd.bind_uniform_buffer(0, 0, _unsorted.instancesCount)
			   .bind_storage_buffer(0, 1, _unsorted.instances)
			   .bind_storage_buffer(0, 2, result.commands)
			   .bind_storage_buffer(0, 3, result.instancesCount)
			   .bind_storage_buffer(0, 4, result.instances)
			   .bind_compute_pipeline("instance_sort_write");
			
			cmd.dispatch_invocations(_unsorted.capacity());
			
		}});
	
	return result;
	
}

auto DrawableInstanceList::frustumCull(Pool& _pool, vuk::RenderGraph& _rg, vuk::Name _name,
	DrawableInstanceList const& _source, MeshBuffer const& _meshes, mat4 _view, mat4 _projection) -> DrawableInstanceList {
	
	auto result = DrawableInstanceList();
	
	// Create a zero-filled command buffer
	
	auto commandsData = ivector<Command>(_source.commands.length());
	result.commands = Buffer<Command>::make(_pool, nameAppend(_name, "commands"),
		vuk::BufferUsageFlagBits::eStorageBuffer | vuk::BufferUsageFlagBits::eIndirectBuffer,
		commandsData, vuk::MemoryUsage::eGPUonly);
	_pool.ptc().dma_task();
	result.commands.attach(_rg, vuk::eHostWrite, vuk::eNone);
	
	// Create destination buffers
	
	auto instancesCountData = 0u; // Zero-initializing instancesCount
	result.instancesCount = Buffer<u32>::make(_pool, nameAppend(_name, "instanceCount"),
		vuk::BufferUsageFlagBits::eStorageBuffer | vuk::BufferUsageFlagBits::eUniformBuffer,
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
			_source.instancesCount.resource(vuk::eComputeRead),
			_source.instances.resource(vuk::eComputeRead),
			result.commands.resource(vuk::eComputeRW),
			result.instancesCount.resource(vuk::eComputeWrite),
			result.instances.resource(vuk::eComputeWrite),
			result.transforms.resource(vuk::eComputeWrite) },
		.execute = [&_source, result, &_meshes, _view, _projection](vuk::CommandBuffer& cmd) {
			
			cmd.bind_storage_buffer(0, 0, _source.commands)
			   .bind_uniform_buffer(0, 1, _source.instancesCount)
			   .bind_storage_buffer(0, 2, _source.instances)
			   .bind_storage_buffer(0, 3, _meshes.descriptorBuf)
			   .bind_storage_buffer(0, 4, result.commands)
			   .bind_storage_buffer(0, 5, result.instancesCount)
			   .bind_storage_buffer(0, 6, result.instances)
			   .bind_storage_buffer(0, 7, result.transforms)
			   .bind_compute_pipeline("frustum_cull");
			
			struct PushConstants {
				mat4 view;
				vec4 frustum;
				u32 commandsCount;
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
					
				}(),
				.commandsCount = u32(_meshes.descriptors.size()) };
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, pushConstants);
			
			cmd.dispatch_invocations(_source.capacity());
			
		}});
	
	return result;
	
}

}

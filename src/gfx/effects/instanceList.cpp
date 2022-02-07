#include "gfx/effects/instanceList.hpp"

#include <cassert>
#include <span>
#include "Tracy.hpp"
#include "base/containers/vector.hpp"
#include "base/util.hpp"
#include "gfx/util.hpp"
#include "tools/modelSchema.hpp"

namespace minote::gfx {

using namespace base;

constexpr auto encodeTransform(ObjectPool::Transform _in) -> InstanceList::Transform {
	
	auto rw = _in.rotation.w();
	auto rx = _in.rotation.x();
	auto ry = _in.rotation.y();
	auto rz = _in.rotation.z();
	
	auto rotationMat = mat3{
		1.0f - 2.0f * (ry * ry + rz * rz),        2.0f * (rx * ry - rw * rz),        2.0f * (rx * rz + rw * ry),
		       2.0f * (rx * ry + rw * rz), 1.0f - 2.0f * (rx * rx + rz * rz),        2.0f * (ry * rz - rw * rx),
		       2.0f * (rx * rz - rw * ry),        2.0f * (ry * rz + rw * rx), 1.0f - 2.0f * (rx * rx + ry * ry)};
	
	rotationMat[0] *= _in.scale.x();
	rotationMat[1] *= _in.scale.y();
	rotationMat[2] *= _in.scale.z();
	
	return to_array({
		vec4(rotationMat[0], _in.position.x()),
		vec4(rotationMat[1], _in.position.y()),
		vec4(rotationMat[2], _in.position.z())});
	
}

auto InstanceList::upload(Pool& _pool, Frame& _frame, vuk::Name _name,
	ObjectPool const& _objects) -> InstanceList {
	
	ZoneScoped;
	
	auto result = InstanceList();
	
	// Precalculate instance count
	
	auto modelCount = 0u;
	auto instanceCount = 0u;
	for (auto idx: iota(ObjectID(0), _objects.size())) {
		
		auto& metadata = _objects.metadata[idx];
		if (!metadata.exists || !metadata.visible)
			continue;
		
		auto id = _objects.modelIDs[idx];
		auto modelIdx = _frame.models.cpu_modelIndices.at(id);
		modelCount += 1;
		instanceCount += _frame.models.cpu_models[modelIdx].meshletCount;
		
	}
	
	// Queue up all valid objects
	
	auto instances = pvector<Instance>();
	instances.reserve(instanceCount);
	auto transforms = pvector<Transform>();
	transforms.reserve(modelCount);
	auto prevTransforms = pvector<Transform>();
	prevTransforms.reserve(modelCount);
	auto colors = pvector<vec4>();
	colors.reserve(modelCount);
	
	result.triangleCount = 0;
	
	for (auto idx: iota(ObjectID(0), _objects.size())) {
		
		auto& metadata = _objects.metadata[idx];
		if (!metadata.exists || !metadata.visible)
			continue;
		
		auto id = _objects.modelIDs[idx];
		auto modelIdx = _frame.models.cpu_modelIndices.at(id);
		auto& model = _frame.models.cpu_models[modelIdx];
		
		// Add meshlet instances
		
		for (auto i: iota(0u, model.meshletCount)) {
			
			instances.push_back(Instance{
				.objectIdx = u32(transforms.size()),
				.meshletIdx = model.meshletOffset + i });
			
			result.triangleCount += divRoundUp(_frame.models.cpu_meshlets[model.meshletOffset + i].indexCount, 3u);
			
		}
		
		// Add model details
		
		transforms.emplace_back(encodeTransform(_objects.transforms[idx]));
		prevTransforms.emplace_back(encodeTransform(_objects.prevTransforms[idx]));
		colors.emplace_back(_objects.colors[idx]);
		
	}
	
	// Upload data to GPU
	
	result.colors = Buffer<vec4>::make(_pool, nameAppend(_name, "colors"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		colors);
	result.transforms = Buffer<Transform>::make(_pool, nameAppend(_name, "transforms"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		transforms);
	result.prevTransforms = Buffer<Transform>::make(_pool, nameAppend(_name, "prevTransforms"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		prevTransforms);
	result.instances = Buffer<Instance>::make(_pool, nameAppend(_name, "instances"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		instances);
	
	result.colors.attach(_frame.rg, vuk::eHostWrite, vuk::eNone);
	result.transforms.attach(_frame.rg, vuk::eHostWrite, vuk::eNone);
	result.prevTransforms.attach(_frame.rg, vuk::eHostWrite, vuk::eNone);
	result.instances.attach(_frame.rg, vuk::eHostWrite, vuk::eNone);
	
	return result;
	
}

void TriangleList::compile(vuk::PerThreadContext& _ptc) {
	
	auto genIndicesPci = vuk::ComputePipelineBaseCreateInfo();
	genIndicesPci.add_spirv(std::vector<u32>{
#include "spv/instanceList/genIndices.comp.spv"
	}, "instanceList/genIndices.comp");
	_ptc.ctx.create_named_pipeline("instanceList/genIndices", genIndicesPci);
	
	auto cullMeshletsPci = vuk::ComputePipelineBaseCreateInfo();
	cullMeshletsPci.add_spirv(std::vector<u32>{
#include "spv/instanceList/cullMeshlets.comp.spv"
	}, "instanceList/cullMeshlets.comp");
	_ptc.ctx.create_named_pipeline("instanceList/cullMeshlets", cullMeshletsPci);
	
}

auto TriangleList::fromInstances(InstanceList _instances, Pool& _pool, Frame& _frame, vuk::Name _name,
	vec3 _cameraPosition) -> TriangleList {
	
	auto result = TriangleList();
	result.colors = _instances.colors;
	result.transforms = _instances.transforms;
	result.prevTransforms = _instances.prevTransforms;
	
	result.instances = Buffer<Instance>::make(_pool, nameAppend(_name, "instances"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		_instances.size());
	result.instances.attach(_frame.rg, vuk::eNone, vuk::eNone);
	
	auto instanceCountData = uvec4{0, 1, 1, 0};
	result.instanceCount = Buffer<uvec4>::make(_frame.framePool, nameAppend(_name, "instanceCount"),
		vuk::BufferUsageFlagBits::eIndirectBuffer |
		vuk::BufferUsageFlagBits::eStorageBuffer,
		std::span(&instanceCountData, 1));
	result.instanceCount.attach(_frame.rg, vuk::eHostWrite, vuk::eNone);
	
	auto groupCounterData = 0u;
	auto groupCounter = Buffer<u32>::make(_frame.framePool, nameAppend(_name, "groupCounter"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		std::span(&groupCounterData, 1));
	
	_frame.rg.add_pass({
		.name = nameAppend(_name, "instanceList/cullMeshlets"),
		.resources = {
			_instances.instances.resource(vuk::eComputeRead),
			_instances.transforms.resource(vuk::eComputeRead),
			result.instanceCount.resource(vuk::eComputeRW),
			result.instances.resource(vuk::eComputeWrite) },
		.execute = [&_frame, _instances, result, groupCounter, _cameraPosition](vuk::CommandBuffer& cmd) {
			
			cmd.bind_storage_buffer(0, 1, _frame.models.meshlets)
			   .bind_storage_buffer(0, 2, _instances.instances)
			   .bind_storage_buffer(0, 3, _instances.transforms)
			   .bind_storage_buffer(0, 4, result.instanceCount)
			   .bind_storage_buffer(0, 5, result.instances)
			   .bind_storage_buffer(0, 6, groupCounter)
			   .bind_compute_pipeline("instanceList/cullMeshlets");
			
			struct CullingData {
				vec3 cameraPosition;
			};
			*cmd.map_scratch_uniform_binding<CullingData>(0, 0) = CullingData{
				.cameraPosition = _cameraPosition };
			
			cmd.specialize_constants(0, tools::MeshletMaxTris);
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, u32(_instances.size()));
			
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
	
	result.indices = Buffer<u32>::make(_pool, _name,
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
		.execute = [result, &_frame, _cameraPosition](vuk::CommandBuffer& cmd) {
			
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
			
			cmd.specialize_constants(0, tools::MeshletMaxTris);
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, _cameraPosition);
			
			cmd.dispatch_indirect(result.instanceCount);
			
		}});
	
	return result;
	
}

}

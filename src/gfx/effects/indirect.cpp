#include "gfx/effects/indirect.hpp"

#include <cstring>
#include <span>
#include "Tracy.hpp"
#include "imgui.h"
#include "vuk/CommandBuffer.hpp"
#include "base/containers/vector.hpp"
#include "base/math.hpp"
#include "base/util.hpp"
#include "gfx/util.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;

void Indirect::compile(vuk::PerThreadContext& _ptc) {
		
	auto cullPci = vuk::ComputePipelineCreateInfo();
	cullPci.add_spirv(std::vector<u32>{
#include "spv/cull.comp.spv"
	}, "cull.comp");
	_ptc.ctx.create_named_pipeline("cull", cullPci);
	
}

Indirect::Indirect(Pool& _pool, vuk::Name _name,
	ObjectPool const& _objects, MeshBuffer const& _meshes) {
	
	ZoneScoped;
	
	// Create the command list
	
	auto commands = ivector<Command>();
	for (auto& descriptor: _meshes.descriptors) {
		
		commands.emplace_back(Command{
			.indexCount = descriptor.indexCount,
			.instanceCount = 0, // counted in the next loop
			.firstIndex = descriptor.indexOffset,
			.vertexOffset = i32(descriptor.vertexOffset),
			.firstInstance = 0 }); // calculated later via prefix sum
		
	}
	
	commandsCount = commands.size();
	
	// Iterate through all valid instances
	
	auto meshIndices = pvector<MeshIndex>(_objects.size());
	auto transforms = pvector<BasicTransform>(_objects.size());
	auto materials = pvector<Material>(_objects.size());
	
	instancesCount = 0;
	for (auto id: iota(ObjectID(0), _objects.size())) {
		
		auto& metadata = _objects.metadata[id];
		if (!metadata.exists || !metadata.visible)
			continue;
		
		auto meshID = _objects.meshIDs[id];
		auto meshIndex = _meshes.descriptorIDs.at(meshID);
		commands[meshIndex].instanceCount += 1;
		
		meshIndices[instancesCount] = meshIndex;
		transforms[instancesCount] = _objects.transforms[id];
		materials[instancesCount] = _objects.materials[id];
		
		instancesCount += 1;
		
	}
	
	meshIndices.resize(instancesCount);
	transforms.resize(instancesCount);
	materials.resize(instancesCount);
	
	// Calculate command list instance offsets (prefix sum)
	
	auto commandOffset = 0_zu;
	
	for (auto i: iota(0_zu, commands.size())) {
		
		auto& command = commands[i];
		command.firstInstance = commandOffset;
		commandOffset += command.instanceCount;
		command.instanceCount = 0;
		
	}
	
	// Clear instance count for GPU culling purposes
	
	for (auto& cmd: commands)
		cmd.instanceCount = 0;
	
	// Create and upload the buffers
	
	commandsBuf = Buffer<Command>::make(_pool, nameAppend(_name, "commands"),
		vuk::BufferUsageFlagBits::eIndirectBuffer | vuk::BufferUsageFlagBits::eStorageBuffer,
		commands);
	
	meshIndicesBuf = Buffer<MeshIndex>::make(_pool, nameAppend(_name, "indices"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		meshIndices);
	transformsBuf = Buffer<BasicTransform>::make(_pool, nameAppend(_name, "transforms"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		transforms);
	materialsBuf = Buffer<Material>::make(_pool, nameAppend(_name, "materials"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		materials);
	
	meshIndicesCulledBuf = Buffer<MeshIndex>::make(_pool, nameAppend(_name, "indicesCulled"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		instancesCount);
	transformsCulledBuf = Buffer<Transform>::make(_pool, nameAppend(_name, "transformsCulled"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		instancesCount);
	materialsCulledBuf = Buffer<Material>::make(_pool, nameAppend(_name, "materialsCulled"),
		vuk::BufferUsageFlagBits::eStorageBuffer,
		instancesCount);
	
	ImGui::Text("Object count: %llu", instancesCount);
	
}

void Indirect::sortAndCull(vuk::RenderGraph& _rg, World const& _world, MeshBuffer const& _meshes) {
	
	auto& view = _world.view;
	auto& projection = _world.projection;
	
	_rg.add_pass({
		.name = "Frustum culling",
		.resources = {
			commandsBuf.resource(vuk::eComputeRW),
			meshIndicesBuf.resource(vuk::eComputeRead),
			transformsBuf.resource(vuk::eComputeRead),
			materialsBuf.resource(vuk::eComputeRead),
			meshIndicesCulledBuf.resource(vuk::eComputeWrite),
			transformsCulledBuf.resource(vuk::eComputeWrite),
			materialsCulledBuf.resource(vuk::eComputeWrite) },
		.execute = [this, &_meshes, view, projection](vuk::CommandBuffer& cmd) {
			
			cmd.bind_storage_buffer(0, 0, commandsBuf)
			   .bind_storage_buffer(0, 1, _meshes.descriptorBuf)
			   .bind_storage_buffer(0, 2, meshIndicesBuf)
			   .bind_storage_buffer(0, 3, transformsBuf)
			   .bind_storage_buffer(0, 4, materialsBuf)
			   .bind_storage_buffer(0, 5, meshIndicesCulledBuf)
			   .bind_storage_buffer(0, 6, transformsCulledBuf)
			   .bind_storage_buffer(0, 7, materialsCulledBuf)
			   .bind_compute_pipeline("cull");
			
			struct CullData {
				mat4 view;
				vec4 frustum;
				u32 instancesCount;
			};
			auto* cullData = cmd.map_scratch_uniform_binding<CullData>(0, 8);
			*cullData = CullData{
				.view = view,
				.frustum = [projection] {
					
					auto projectionT = transpose(projection);
					vec4 frustumX = projectionT[3] + projectionT[0];
					vec4 frustumY = projectionT[3] + projectionT[1];
					frustumX /= length(vec3(frustumX));
					frustumY /= length(vec3(frustumY));
					return vec4{frustumX.x(), frustumX.z(), frustumY.y(), frustumY.z()};
				
				}(),
				.instancesCount = u32(instancesCount) };
			
			cmd.dispatch_invocations(instancesCount);
			
		}});
	
	commandsBuf.attach(_rg, vuk::eTransferDst, vuk::eNone);
	meshIndicesBuf.attach(_rg, vuk::eTransferDst, vuk::eNone);
	transformsBuf.attach(_rg, vuk::eTransferDst, vuk::eNone);
	materialsBuf.attach(_rg, vuk::eTransferDst, vuk::eNone);
	meshIndicesCulledBuf.attach(_rg, vuk::eNone, vuk::eNone);
	transformsCulledBuf.attach(_rg, vuk::eNone, vuk::eNone);
	materialsCulledBuf.attach(_rg, vuk::eNone, vuk::eNone);
	
}

}

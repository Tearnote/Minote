#include "gfx/modules/indirect.hpp"

#include <cstring>
#include <span>
#include "Tracy.hpp"
#include "imgui.h"
#include "vuk/CommandBuffer.hpp"
#include "base/containers/array.hpp"
#include "base/math.hpp"
#include "base/util.hpp"
#include "gfx/util.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;

Indirect::Indirect(vuk::PerThreadContext& _ptc, vuk::Name _name,
	ObjectPool const& _objects, MeshBuffer const& _meshes) {
	
	ZoneScoped;
	
	// Create the command list
	
	auto commands = ivector<VkDrawIndexedIndirectCommand>();
	for (auto& descriptor: _meshes.descriptors) {
		
		commands.emplace_back(VkDrawIndexedIndirectCommand{
			.indexCount = descriptor.indexCount,
			.instanceCount = 0, // counted in the next loop
			.firstIndex = descriptor.indexOffset,
			.vertexOffset = i32(descriptor.vertexOffset),
			.firstInstance = 0 }); // calculated later via prefix sum
		
	}
	
	commandsCount = commands.size();
	
	// Iterate through all valid instances
	
	auto meshIndices = array<u32>(_objects.size());
	auto transforms = array<ObjectPool::Transform>(_objects.size());
	auto materials = array<ObjectPool::Material>(_objects.size());
	
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
	
	commandsBuf = Buffer<VkDrawIndexedIndirectCommand>(_ptc,
		nameAppend(_name, "commands"), commands,
		vuk::BufferUsageFlagBits::eIndirectBuffer | vuk::BufferUsageFlagBits::eStorageBuffer);
	
	meshIndexBuf = Buffer<u32>(_ptc,
		nameAppend(_name, "indices"), meshIndices,
		vuk::BufferUsageFlagBits::eStorageBuffer);
	transformBuf = Buffer<ObjectPool::Transform>(_ptc,
		nameAppend(_name, "transforms"), transforms,
		vuk::BufferUsageFlagBits::eStorageBuffer);
	materialBuf = Buffer<ObjectPool::Material>(_ptc,
		nameAppend(_name, "materials"), materials,
		vuk::BufferUsageFlagBits::eStorageBuffer);
	
	meshIndexCulledBuf = Buffer<u32>(_ptc,
		nameAppend(_name, "indicesCulled"),
		vuk::BufferUsageFlagBits::eStorageBuffer, instancesCount);
	transformCulledBuf = Buffer<vec4[3]>(_ptc,
		nameAppend(_name, "transformsCulled"),
		vuk::BufferUsageFlagBits::eStorageBuffer, instancesCount);
	materialCulledBuf = Buffer<ObjectPool::Material>(_ptc,
		nameAppend(_name, "materialsCulled"),
		vuk::BufferUsageFlagBits::eStorageBuffer, instancesCount);
	
	ImGui::Text("Object count: %llu", instancesCount);
	
	// Prepare the culling shader
	
	if (!pipelinesCreated) {
		
		auto cullPci = vuk::ComputePipelineCreateInfo();
		cullPci.add_spirv(std::vector<u32>{
#include "spv/cull.comp.spv"
		}, "cull.comp");
		_ptc.ctx.create_named_pipeline("cull", cullPci);
		
		pipelinesCreated = true;
		
	}
	
}

auto Indirect::sortAndCull(World const& _world, MeshBuffer const& _meshes) -> vuk::RenderGraph {
	
	auto rg = vuk::RenderGraph();
	
	auto& view = _world.view;
	auto& projection = _world.projection;
	
	rg.add_pass({
		.name = "Frustum culling",
		.resources = {
			vuk::Resource(Commands_n,        vuk::Resource::Type::eBuffer, vuk::eComputeRW),
			vuk::Resource(MeshIndex_n,       vuk::Resource::Type::eBuffer, vuk::eComputeRead),
			vuk::Resource(Transform_n,       vuk::Resource::Type::eBuffer, vuk::eComputeRead),
			vuk::Resource(Material_n,        vuk::Resource::Type::eBuffer, vuk::eComputeRead),
			vuk::Resource(MeshIndexCulled_n, vuk::Resource::Type::eBuffer, vuk::eComputeWrite),
			vuk::Resource(TransformCulled_n, vuk::Resource::Type::eBuffer, vuk::eComputeWrite),
			vuk::Resource(MaterialCulled_n,  vuk::Resource::Type::eBuffer, vuk::eComputeWrite),
		},
		.execute = [this, &_meshes, view, projection](vuk::CommandBuffer& cmd) {
			cmd.bind_storage_buffer(0, 0, commandsBuf)
			   .bind_storage_buffer(0, 1, _meshes.descriptorBuf)
			   .bind_storage_buffer(0, 2, meshIndexBuf)
			   .bind_storage_buffer(0, 3, transformBuf)
			   .bind_storage_buffer(0, 4, materialBuf)
			   .bind_storage_buffer(0, 5, meshIndexCulledBuf)
			   .bind_storage_buffer(0, 6, transformCulledBuf)
			   .bind_storage_buffer(0, 7, materialCulledBuf)
			   .bind_compute_pipeline("cull");
			
			struct CullData {
				mat4 view;
				vec4 frustum;
				u32 instancesCount;
			};
			auto* cullData = cmd.map_scratch_uniform_binding<CullData>(0, 8);
			*cullData = CullData{
				.view = view,
				.frustum = [this, projection] {
					
					auto projectionT = transpose(projection);
					vec4 frustumX = projectionT[3] + projectionT[0];
					vec4 frustumY = projectionT[3] + projectionT[1];
					frustumX /= length(vec3(frustumX));
					frustumY /= length(vec3(frustumY));
					return vec4{frustumX.x(), frustumX.z(), frustumY.y(), frustumY.z()};
				
				}(),
				.instancesCount = u32(instancesCount) };
			
			cmd.dispatch_invocations(instancesCount);
		},
	});
	
	rg.attach_buffer(Commands_n,
		commandsBuf,
		vuk::eTransferDst,
		vuk::eNone);
	rg.attach_buffer(MeshIndex_n,
		meshIndexBuf,
		vuk::eTransferDst,
		vuk::eNone);
	rg.attach_buffer(Transform_n,
		transformBuf,
		vuk::eTransferDst,
		vuk::eNone);
	rg.attach_buffer(Material_n,
		materialBuf,
		vuk::eTransferDst,
		vuk::eNone);
	rg.attach_buffer(MeshIndexCulled_n,
		meshIndexCulledBuf,
		vuk::eNone,
		vuk::eNone);
	rg.attach_buffer(TransformCulled_n,
		transformCulledBuf,
		vuk::eNone,
		vuk::eNone);
	rg.attach_buffer(MaterialCulled_n,
		materialCulledBuf,
		vuk::eNone,
		vuk::eNone);
	
	return rg;
	
}

}

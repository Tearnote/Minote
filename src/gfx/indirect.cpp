#include "gfx/indirect.hpp"

#include <cstring>
#include "vuk/CommandBuffer.hpp"
#include "base/hashmap.hpp"
#include "base/math.hpp"
#include "base/util.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;

Indirect::Indirect(vuk::PerThreadContext& _ptc,
	Objects const& _objects, Meshes const& _meshes) {
	
	// Create the command list
	
	auto sortedMeshIDs = hashmap<ID, u32>();
	auto commands = std::vector<Command>();
	commands.reserve(_meshes.size());
	
	for (auto id = ObjectID(0); id < _objects.size(); id += 1) {
		
		auto& metadata = _objects.metadata[id];
		if (!metadata.exists || !metadata.visible)
			continue;
		
		auto meshID = _objects.meshIDs[id];
		if (!sortedMeshIDs.contains(meshID)) {
			
			auto& descriptor = _meshes.at(meshID);
			sortedMeshIDs.emplace(meshID, commands.size());
			commands.emplace_back(Command{
				.indexCount = descriptor.indexCount,
				.instanceCount = 0, // counted during the loop
				.firstIndex = descriptor.indexOffset,
				.vertexOffset = i32(descriptor.vertexOffset),
				.firstInstance = 0, // calculated later
				.meshRadius = descriptor.radius,
			});
			
		}
		
		commands[sortedMeshIDs.at(meshID)].instanceCount += 1;
		
	}
	
	// Calculate command list instance offsets
	
	auto commandOffset = 0_zu;
	for (auto i = 0_zu; i < commands.size(); i += 1) {
		
		auto& command = commands[i];
		command.firstInstance = commandOffset;
		commandOffset += command.instanceCount;
		command.instanceCount = 0;
		
	}
	
	// Create the instance vector sorted by mesh ID
	
	auto sortedInstances = std::vector<Instance>(commandOffset);
	for (auto id = ObjectID(0); id < _objects.size(); id += 1) {
		
		auto& metadata = _objects.metadata[id];
		if (!metadata.exists || !metadata.visible)
			continue;
		
		auto meshID = _objects.meshIDs[id];
		auto meshIndex = sortedMeshIDs.at(meshID);
		auto& command = commands[meshIndex];
		auto instanceIndex = command.firstInstance + command.instanceCount;
		auto& material = _objects.materials[id];
		sortedInstances[instanceIndex] = Instance{
			.transform = _objects.transforms[id],
			.tint = material.tint,
			.roughness = material.roughness,
			.metalness = material.metalness,
			.meshID = meshIndex,
		};
		
		command.instanceCount += 1;
		
	}
	
	// Clear instance count for GPU culling
	
	for (auto& cmd: commands)
		cmd.instanceCount = 0;
	
	// Create and upload the buffers
	
	commandsCount = commands.size();
	commandsBuf = _ptc.allocate_scratch_buffer(
		vuk::MemoryUsage::eCPUtoGPU,
		vuk::BufferUsageFlagBits::eIndirectBuffer | vuk::BufferUsageFlagBits::eStorageBuffer,
		sizeof(Command) * commands.size(), alignof(Command));
	std::memcpy(commandsBuf.mapped_ptr, commands.data(), sizeof(Command) * commands.size());
	
	instancesCount = sortedInstances.size();
	instancesBuf = _ptc.allocate_scratch_buffer(
		vuk::MemoryUsage::eCPUtoGPU,
		vuk::BufferUsageFlagBits::eStorageBuffer,
		sizeof(Instance) * sortedInstances.size(), alignof(Instance));
	std::memcpy(instancesBuf.mapped_ptr, sortedInstances.data(), sizeof(Instance) * sortedInstances.size());
	
	instancesCulledBuf = _ptc.allocate_scratch_buffer(
		vuk::MemoryUsage::eGPUonly,
		vuk::BufferUsageFlagBits::eStorageBuffer,
		sizeof(Instance) * sortedInstances.size(), alignof(Instance));
	
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

auto Indirect::frustumCull(World const& _world) -> vuk::RenderGraph {
	
	auto rg = vuk::RenderGraph();
	
	auto& view = _world.view;
	auto& projection = _world.projection;
	
	rg.add_pass({
		.name = "Frustum culling",
		.resources = {
			"commands"_buffer(vuk::eComputeRW),
			"instances"_buffer(vuk::eComputeRead),
			"instances_culled"_buffer(vuk::eComputeWrite),
		},
		.execute = [this, view, projection](vuk::CommandBuffer& cmd) {
			auto commandsBuf = cmd.get_resource_buffer("commands");
			auto instancesBuf = cmd.get_resource_buffer("instances");
			auto instancesCulledBuf = cmd.get_resource_buffer("instances_culled");
			cmd.bind_storage_buffer(0, 0, commandsBuf)
			   .bind_storage_buffer(0, 1, instancesBuf)
			   .bind_storage_buffer(0, 2, instancesCulledBuf)
			   .bind_compute_pipeline("cull");
			struct CullData {
				mat4 view;
				vec4 frustum;
				u32 instancesCount;
			};
			auto* cullData = cmd.map_scratch_uniform_binding<CullData>(0, 3);
			*cullData = CullData{
				.view = view,
				.frustum = [this, projection] {
					auto projectionT = transpose(projection);
					vec4 frustumX = projectionT[3] + projectionT[0];
					vec4 frustumY = projectionT[3] + projectionT[1];
					frustumX /= length(vec3(frustumX));
					frustumY /= length(vec3(frustumY));
					return vec4(frustumX.x, frustumX.z, frustumY.y, frustumY.z);
				}(),
				.instancesCount = u32(instancesCount),
			};
			cmd.dispatch_invocations(instancesCount);
		},
	});
	
	rg.attach_buffer("commands", commandsBuf, vuk::eTransferDst, {});
	rg.attach_buffer("instances", instancesBuf, vuk::eTransferDst, {});
	rg.attach_buffer("instances_culled", instancesCulledBuf, {}, {});
	
	return rg;
	
}

}

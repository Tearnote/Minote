#include "gfx/indirect.hpp"

#include <cstring>
#include "base/hashmap.hpp"
#include "base/util.hpp"

namespace minote::gfx {

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
	
}

}

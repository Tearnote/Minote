#include "gfx/module/indirect.hpp"

#include <cstring>
#include "optick.h"
#include "vuk/CommandBuffer.hpp"
#include "base/container/hashmap.hpp"
#include "base/math.hpp"
#include "base/util.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;

Indirect::Indirect(vuk::PerThreadContext& _ptc,
	Objects const& _objects, Meshes const& _meshes) {
	
	OPTICK_EVENT("Indirect::Indirect");
	
	// Create the command list
	
	auto commands = std::vector<Command>();
	{
		OPTICK_EVENT("Create command list");
		
		commands.reserve(_meshes.size());
		for (auto& descriptor: _meshes.descriptors) {
			
			commands.emplace_back(Command{
				.indexCount = descriptor.indexCount,
				.instanceCount = 0, // counted during the next loop
				.firstIndex = descriptor.indexOffset,
				.vertexOffset = i32(descriptor.vertexOffset),
				.firstInstance = 0, // calculated later
				.meshRadius = descriptor.radius});
			
		}
	}
	
	// Count instances per mesh
	
	{
		OPTICK_EVENT("Count instances per mesh");
		
		for (auto size = _objects.size(), id = ObjectID(0); id < size; id += 1) {
			
			auto& metadata = _objects.metadata[id];
			if (!metadata.exists || !metadata.visible)
				continue;
			
			auto meshID = _objects.meshIDs[id];
			auto meshIndex = _meshes.descriptorIDs.at(meshID);
			commands[meshIndex].instanceCount += 1;
			
		}
	}
	
	// Calculate command list instance offsets
	
	auto commandOffset = 0_zu;
	{
		OPTICK_EVENT("Calculate command list instance offsets");
		
		for (auto size = commands.size(), i = 0_zu; i < size; i += 1) {
			
			auto& command = commands[i];
			command.firstInstance = commandOffset;
			commandOffset += command.instanceCount;
			command.instanceCount = 0;
			
		}
	}
	
	// Create the instance vector sorted by mesh ID
	
	auto sortedInstances = std::vector<Instance>(commandOffset);
	{
		OPTICK_EVENT("Create the instance vector");
		
		for (auto size = _objects.size(), id = ObjectID(0); id < size; id += 1) {
			
			auto& metadata = _objects.metadata[id];
			if (!metadata.exists || !metadata.visible)
				continue;
			
			auto meshID = _objects.meshIDs[id];
			auto meshIndex = _meshes.descriptorIDs.at(meshID);
			auto& command = commands[meshIndex];
			auto instanceIndex = command.firstInstance + command.instanceCount;
			auto& material = _objects.materials[id];
			sortedInstances[instanceIndex] = Instance{
				.transform = _objects.transforms[id],
				.tint = material.tint,
				.roughness = material.roughness,
				.metalness = material.metalness,
				.meshID = u32(meshIndex),
			};
			
			command.instanceCount += 1;
			
		}
	}
	
	// Clear instance count for GPU culling
	
	{
		OPTICK_EVENT("Clear instance count");
		
		for (auto& cmd: commands)
			cmd.instanceCount = 0;
	}
	
	// Create and upload the buffers
	
	{
		OPTICK_EVENT("Upload buffers");
		
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
			vuk::Resource(Commands_n,        vuk::Resource::Type::eBuffer, vuk::eComputeRW),
			vuk::Resource(Instances_n,       vuk::Resource::Type::eBuffer, vuk::eComputeRead),
			vuk::Resource(InstancesCulled_n, vuk::Resource::Type::eBuffer, vuk::eComputeWrite),
		},
		.execute = [this, view, projection](vuk::CommandBuffer& cmd) {
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
					return vec4{frustumX.x(), frustumX.z(), frustumY.y(), frustumY.z()};
				}(),
				.instancesCount = u32(instancesCount),
			};
			cmd.dispatch_invocations(instancesCount);
		},
	});
	
	rg.attach_buffer(Commands_n,
		commandsBuf,
		vuk::eTransferDst,
		vuk::eNone);
	rg.attach_buffer(Instances_n,
		instancesBuf,
		vuk::eTransferDst,
		vuk::eNone);
	rg.attach_buffer(InstancesCulled_n,
		instancesCulledBuf,
		vuk::eNone,
		vuk::eNone);
	
	return rg;
	
}

}

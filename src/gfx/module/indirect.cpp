#include "gfx/module/indirect.hpp"

#include <cstring>
#include "optick.h"
#include "imgui.h"
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
	
	// Count instances per mesh
	
	for (auto size = _objects.size(), id = ObjectID(0); id < size; id += 1) {
		
		auto& metadata = _objects.metadata[id];
		if (!metadata.exists || !metadata.visible)
			continue;
		
		auto meshIndex = _objects.meshIndex[id];
		commands[meshIndex].instanceCount += 1;
		
	}
	
	// Calculate command list instance offsets
	
	auto commandOffset = 0_zu;
	
	for (auto size = commands.size(), i = 0_zu; i < size; i += 1) {
		
		auto& command = commands[i];
		command.firstInstance = commandOffset;
		commandOffset += command.instanceCount;
		command.instanceCount = 0;
		
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
	
	instancesCount = _objects.size();
	auto createAndUpload = [this, &_ptc]<typename T>(std::vector<T> const& _data) {
		
		auto buf = _ptc.allocate_scratch_buffer(
			vuk::MemoryUsage::eCPUtoGPU,
			vuk::BufferUsageFlagBits::eStorageBuffer,
			sizeof(T) * instancesCount, alignof(T));
		std::memcpy(buf.mapped_ptr, _data.data(), sizeof(T) * instancesCount);
		return buf;
		
	};
	auto createEmpty = [this, &_ptc]<typename T>(std::vector<T> const&) {
		
		return _ptc.allocate_scratch_buffer(
			vuk::MemoryUsage::eGPUonly,
			vuk::BufferUsageFlagBits::eStorageBuffer,
			sizeof(T) * instancesCount, alignof(T));
		
	};
	
	metadataBuf      = createAndUpload(_objects.metadata);
	meshIndexBuf     = createAndUpload(_objects.meshIndex);
	transformBuf     = createAndUpload(_objects.transform);
	prevTransformBuf = createAndUpload(_objects.prevTransform);
	materialBuf      = createAndUpload(_objects.material);
	
	transformCulledBuf     = createEmpty(_objects.transform);
	prevTransformCulledBuf = createEmpty(_objects.prevTransform);
	materialCulledBuf      = createEmpty(_objects.material);
	
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

auto Indirect::frustumCull(World const& _world) -> vuk::RenderGraph {
	
	auto rg = vuk::RenderGraph();
	
	auto& view = _world.view;
	auto& projection = _world.projection;
	
	rg.add_pass({
		.name = "Frustum culling",
		.resources = {
			vuk::Resource(Commands_n,            vuk::Resource::Type::eBuffer, vuk::eComputeRW),
			vuk::Resource(Metadata_n,            vuk::Resource::Type::eBuffer, vuk::eComputeRead),
			vuk::Resource(MeshIndex_n,           vuk::Resource::Type::eBuffer, vuk::eComputeRead),
			vuk::Resource(Transform_n,           vuk::Resource::Type::eBuffer, vuk::eComputeRead),
			vuk::Resource(PrevTransform_n,       vuk::Resource::Type::eBuffer, vuk::eComputeRead),
			vuk::Resource(Material_n,            vuk::Resource::Type::eBuffer, vuk::eComputeRead),
			vuk::Resource(TransformCulled_n,     vuk::Resource::Type::eBuffer, vuk::eComputeWrite),
			vuk::Resource(PrevTransformCulled_n, vuk::Resource::Type::eBuffer, vuk::eComputeWrite),
			vuk::Resource(MaterialCulled_n,      vuk::Resource::Type::eBuffer, vuk::eComputeWrite),
		},
		.execute = [this, view, projection](vuk::CommandBuffer& cmd) {
			cmd.bind_storage_buffer(0, 0, commandsBuf)
			   .bind_storage_buffer(0, 1, metadataBuf)
			   .bind_storage_buffer(0, 2, meshIndexBuf)
			   .bind_storage_buffer(0, 3, transformBuf)
			   .bind_storage_buffer(0, 4, prevTransformBuf)
			   .bind_storage_buffer(0, 5, materialBuf)
			   .bind_storage_buffer(0, 6, transformCulledBuf)
			   .bind_storage_buffer(0, 7, prevTransformCulledBuf)
			   .bind_storage_buffer(0, 8, materialCulledBuf)
			   .bind_compute_pipeline("cull");
			struct CullData {
				mat4 view;
				vec4 frustum;
				u32 instancesCount;
			};
			auto* cullData = cmd.map_scratch_uniform_binding<CullData>(0, 9);
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
	rg.attach_buffer(Metadata_n,
		metadataBuf,
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
	rg.attach_buffer(PrevTransform_n,
		prevTransformBuf,
		vuk::eTransferDst,
		vuk::eNone);
	rg.attach_buffer(Material_n,
		materialBuf,
		vuk::eTransferDst,
		vuk::eNone);
	rg.attach_buffer(TransformCulled_n,
		transformCulledBuf,
		vuk::eNone,
		vuk::eNone);
	rg.attach_buffer(PrevTransformCulled_n,
		prevTransformCulledBuf,
		vuk::eNone,
		vuk::eNone);
	rg.attach_buffer(MaterialCulled_n,
		materialCulledBuf,
		vuk::eNone,
		vuk::eNone);
	
	return rg;
	
}

}

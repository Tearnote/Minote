#include "gfx/indirect.hpp"

#include <cstring>
#include "base/util.hpp"

namespace minote::gfx {

using namespace base::literals;

auto Indirect::createBuffers(vuk::PerThreadContext& ptc, Meshes const& meshes, Instances const& _instances) -> Indirect {
	auto commandsVec = std::vector<Command>();
	auto instancesVec = std::vector<Instance>();

	commandsVec.reserve(meshes.size());
	auto totalInstanceCount = 0_zu;
	for (auto& [id, vec]: _instances.instances)
		totalInstanceCount += vec.size();
	instancesVec.reserve(totalInstanceCount);

	for (auto& [id, vec]: _instances.instances) {
		auto& descriptor = meshes.at(id);
		commandsVec.emplace_back(Command{
			.indexCount = descriptor.indexCount,
			.instanceCount = u32(vec.size()),
			.firstIndex = descriptor.indexOffset,
			.vertexOffset = i32(descriptor.vertexOffset),
			.firstInstance = u32(instancesVec.size()),
			.meshRadius = descriptor.radius,
		});

		auto start = instancesVec.end();
		instancesVec.insert(instancesVec.end(), vec.begin(), vec.end());

		for (auto it = start; it < instancesVec.end(); it += 1)
			it->meshID = meshes.descriptorIDs.at(id);
	}

	auto result = Indirect{
		.commands = ptc.allocate_scratch_buffer(
			vuk::MemoryUsage::eCPUtoGPU, vuk::BufferUsageFlagBits::eIndirectBuffer,
			sizeof(Command) * commandsVec.size(), alignof(Command)),
		.commandsCount = commandsVec.size(),
		.instances = ptc.allocate_scratch_buffer(
			vuk::MemoryUsage::eCPUtoGPU, vuk::BufferUsageFlagBits::eStorageBuffer,
			sizeof(Instance) * instancesVec.size(), alignof(Instance)),
		.instancesCount = instancesVec.size(),
	};

	std::memcpy(result.commands.mapped_ptr, commandsVec.data(), sizeof(Command) * commandsVec.size());
	std::memcpy(result.instances.mapped_ptr, instancesVec.data(), sizeof(Instance) * instancesVec.size());

	return result;
}

}

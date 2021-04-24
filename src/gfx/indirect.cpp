#include "gfx/indirect.hpp"

#include <cstring>
#include "base/util.hpp"

namespace minote::gfx {

using namespace base::literals;

auto Indirect::createBuffers(vuk::PerThreadContext& ptc, Meshes const& meshes, Instances const& _instances) -> Indirect {
	auto totalInstanceCount = 0_zu;
	for (auto& [id, vec]: _instances.instances)
		totalInstanceCount += vec.size();

	auto result = Indirect{
		.commands = ptc.allocate_scratch_buffer(
			vuk::MemoryUsage::eCPUtoGPU,
			vuk::BufferUsageFlagBits::eIndirectBuffer | vuk::BufferUsageFlagBits::eStorageBuffer,
			sizeof(Command) * _instances.size(), alignof(Command)),
		.commandsCount = _instances.size(),
		.instances = ptc.allocate_scratch_buffer(
			vuk::MemoryUsage::eCPUtoGPU,
			vuk::BufferUsageFlagBits::eStorageBuffer,
			sizeof(Instance) * totalInstanceCount, alignof(Instance)),
		.instancesCulled = ptc.allocate_scratch_buffer(
			vuk::MemoryUsage::eGPUonly,
			vuk::BufferUsageFlagBits::eStorageBuffer,
			sizeof(Instance) * totalInstanceCount, alignof(Instance)),
		.instancesCount = totalInstanceCount,
	};

	auto instanceOffset = 0_zu;
	auto* instanceIt = (Instance*)(result.instances.mapped_ptr);
	auto* commandIt = (Command*)(result.commands.mapped_ptr);
	for (auto& [id, vec]: _instances.instances) {
		auto& descriptor = meshes.at(id);
		*commandIt = Command{
			.indexCount = descriptor.indexCount,
			.instanceCount = u32(vec.size()),
			.firstIndex = descriptor.indexOffset,
			.vertexOffset = i32(descriptor.vertexOffset),
			.firstInstance = u32(instanceOffset),
			.meshRadius = descriptor.radius,
		};
		commandIt += 1;

		std::memcpy(instanceIt, vec.data(), sizeof(Instance) * vec.size());
		for (auto i = 0_zu; i < vec.size(); i += 1)
			instanceIt[i].meshID = meshes.descriptorIDs.at(id);
		instanceIt += vec.size();
		instanceOffset += vec.size();
	}

	return result;
}

}

#include "gfx/instances.hpp"

#include "base/types.hpp"
#include "base/util.hpp"

namespace minote::gfx {

using namespace base::literals;

void Instances::addInstances(ID mesh, std::span<Instance const> _instances) {
	if (!instances.contains(mesh))
		instances.emplace(mesh, decltype(instances)::mapped_type());

	auto& vec = instances.at(mesh);
	vec.insert(vec.end(), _instances.begin(), _instances.end());
}
auto Instances::makeIndirect(Meshes const& meshes)
	-> std::pair<std::vector<vuk::DrawIndexedIndirectCommand>, std::vector<Instance>> {
	auto commands = std::vector<vuk::DrawIndexedIndirectCommand>();
	auto allInstances = std::vector<Instance>();

	commands.reserve(instances.size());
	auto totalInstanceCount = 0_zu;
	for (auto& [id, vec]: instances)
		totalInstanceCount += vec.size();
	allInstances.reserve(totalInstanceCount);

	for (auto& [id, vec]: instances) {
		// Insert the meshID for compute purposes
		for(auto& inst: vec)
			inst.meshID = meshes.descriptorIDs.at(id);

		auto& descriptor = meshes.at(id);
		commands.emplace_back(vuk::DrawIndexedIndirectCommand{
			.indexCount = descriptor.indexCount,
			.instanceCount = u32(vec.size()),
			.firstIndex = descriptor.indexOffset,
			.vertexOffset = i32(descriptor.vertexOffset),
			.firstInstance = u32(allInstances.size()),
		});
		allInstances.insert(allInstances.end(), vec.begin(), vec.end());
	}

	instances.clear();
	return std::make_pair(std::move(commands), std::move(allInstances));
}

}

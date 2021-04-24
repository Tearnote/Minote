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

}

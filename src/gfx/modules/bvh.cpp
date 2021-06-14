#include "gfx/modules/bvh.hpp"

#include <cassert>
#include <vector>
#include "bvh/sweep_sah_builder.hpp"
#include "bvh/utilities.hpp"
#include "bvh/triangle.hpp"
#include "bvh/vector.hpp"
#include "bvh/bvh.hpp"
#include "base/log.hpp"

namespace minote::gfx::modules {

void Bvh::generateMeshesBvh(Meshes const& _meshes) {
	
	using Vector3  = bvh::Vector3<float>;
	using Triangle = bvh::Triangle<float>;
	
	for (auto& [id, _]: _meshes.descriptorIDs) {
		
		auto& descriptor = _meshes.at(id);
		
		auto triangles = std::vector<Triangle>();
		triangles.reserve(descriptor.indexCount);
		assert(descriptor.indexCount % 3 == 0);
		
		for (auto i = 0u; i < descriptor.indexCount; i += 3) {
			
			auto vertex = [&](u32 _index) {
				
				auto element = _meshes.indices[_index + descriptor.indexOffset];
				auto vertexIdx = element + descriptor.vertexOffset;
				return Vector3(
					_meshes.vertices[vertexIdx].x,
					_meshes.vertices[vertexIdx].y,
					_meshes.vertices[vertexIdx].z);
				
			};
			
			triangles.emplace_back(
				vertex(i  ),
				vertex(i+1),
				vertex(i+2));
			
		}
		
		auto bvh = bvh::Bvh<float>();
		auto builder = bvh::SweepSahBuilder(bvh);
		builder.max_leaf_size = 2;
		auto globalAABB = bvh::BoundingBox(
			Vector3(
				descriptor.aabbMin.x,
				descriptor.aabbMin.y,
				descriptor.aabbMin.z),
			Vector3(
				descriptor.aabbMax.x,
				descriptor.aabbMax.y,
				descriptor.aabbMax.z));
		auto [aabbs, centers] = bvh::compute_bounding_boxes_and_centers(triangles.data(), triangles.size());
		builder.build(globalAABB, aabbs.get(), centers.get(), triangles.size());
		
		for (auto i = 0u; i < bvh.node_count; i += 1) {
			//L.trace("Node {}: count {}, first child/prim {}", i, bvh.nodes[i].primitive_count, bvh.nodes[i].first_child_or_primitive);
		}
		
		L.trace("BVH generation complete: {}", +id);
		
	}
	
}

}

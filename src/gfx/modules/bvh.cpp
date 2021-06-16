#include "gfx/modules/bvh.hpp"

#include <cassert>
#include <vector>
#include "bvh/sweep_sah_builder.hpp"
#include "bvh/utilities.hpp"
#include "bvh/triangle.hpp"
#include "bvh/vector.hpp"
#include "bvh/bvh.hpp"
#include "base/util.hpp"

namespace minote::gfx::modules {

using namespace base::literals;

void Bvh::generateMeshesBvh(vuk::PerThreadContext& _ptc, Meshes const& _meshes) {
	
	auto depthFirstBvh = std::vector<Node>();
	auto bvhDescriptors = std::vector<Descriptor>();
	
	using Vector3  = bvh::Vector3<float>;
	using Triangle = bvh::Triangle<float>;
	
	for (auto& descriptor: _meshes.descriptors) {
		
		// Generate list of triangle primitives
		
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
		
		// Build the BVH
		
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
		
		// Establish depth-first ordering
		
		auto totalDepthFirstNodes = 0_zu;
		auto depthFirstOrder = std::vector<usize>(bvh.node_count);
		{
			auto stack = std::vector<usize>();
			auto counter = 0_zu;
			auto nodeIndex = 0_zu;
			while (true) {
				
				auto& node = bvh.nodes[nodeIndex];
				depthFirstOrder[nodeIndex] = counter;
				counter += 1;
				counter += node.primitive_count == 2? 2 : 0;
				
				if (!node.is_leaf()) {
					
					nodeIndex = node.first_child_or_primitive;
					stack.emplace_back(node.first_child_or_primitive + 1);
					
				} else {
					
					if (stack.empty())
						break;
					nodeIndex = stack.back();
					stack.pop_back();
					
				}
				
			}
			
			totalDepthFirstNodes = counter;
			
		}
		
		// Build depth-first BVH
		
		{
			struct StackLink {
				
				usize index;
				usize miss;
				
			};
			
			auto stack = std::vector<StackLink>();
			auto missLink = -1_zu;
			auto nodeIndex = 0_zu;
			auto offset = depthFirstBvh.size();
			
			bvhDescriptors.emplace_back(Descriptor{
				.offset = u32(offset),
				.nodeCount = u32(totalDepthFirstNodes) });
			depthFirstBvh.resize(depthFirstBvh.size() + totalDepthFirstNodes);
			
			while (true) {
				
				auto& node = bvh.nodes[nodeIndex];
				auto depthFirstIndex = depthFirstOrder[nodeIndex];
				
				// Create internal node
				
				if (node.primitive_count != 1) {
					
					depthFirstBvh[offset + depthFirstIndex] = Node{
						.inter = {
							.aabbMin = vec3(
								node.bounds[0],
								node.bounds[1],
								node.bounds[2]),
							.isLeaf = 0u,
							.aabbMax = vec3(
								node.bounds[3],
								node.bounds[4],
								node.bounds[5]),
							.miss = u32(missLink) }};
					assert(missLink > depthFirstIndex);
					
					depthFirstIndex += 1;
					
				}
				
				// Create leaf node(s)
				
				for (auto i = 0_zu; i < node.primitive_count; i += 1) {
					
					auto primitive = node.first_child_or_primitive + i;
					auto index = primitive * 3 + descriptor.indexOffset;
					auto miss = depthFirstIndex + i + 1;
					if (miss == depthFirstBvh.size())
						miss = -1_zu;
					
					depthFirstBvh[offset + depthFirstIndex + i] = Node{
						.leaf = {
							.indices = uvec3(
								_meshes.indices[index  ],
								_meshes.indices[index+1],
								_meshes.indices[index+2]),
							.isLeaf = 1u,
							.miss = u32(miss) }};
					
				}
				
				if (!node.is_leaf()) {
					
					// Internal node, enter first child and put second child on stack
					
					nodeIndex = node.first_child_or_primitive;
					stack.emplace_back(StackLink{
						.index = node.first_child_or_primitive + 1,
						.miss = missLink });
					missLink = depthFirstOrder[bvh.sibling(nodeIndex)];
					
				} else {
					
					// Leaf node, get next task from the top of stack
					
					if (stack.empty())
						break;
					auto& back = stack.back();
					nodeIndex = back.index;
					missLink = back.miss;
					stack.pop_back();
					
				}
				
			}
			
		}
		
	}
	
	// Upload to GPU
	
	bvhBuf = _ptc.create_buffer<Node>(vuk::MemoryUsage::eGPUonly,
		vuk::BufferUsageFlagBits::eStorageBuffer, std::span(depthFirstBvh)).first;
	descriptorsBuf = _ptc.create_buffer<Descriptor>(vuk::MemoryUsage::eGPUonly,
		vuk::BufferUsageFlagBits::eStorageBuffer, std::span(bvhDescriptors)).first;
	
}

}

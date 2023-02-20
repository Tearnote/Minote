#include <iterator>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <array>
#include <functional>
#include "meshoptimizer.h"
#include "mpack/mpack.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#include "stx/vector.hpp"
#include "stx/ranges.hpp"
#include "stx/verify.hpp"
#include "stx/except.hpp"
#include "math.hpp"
#include "stx/defer.hpp"
#include "tools/modelSchema.hpp"

using namespace minote;
using namespace type_literals;

struct Worknode {
	cgltf_node* node;
	float4x4 parentTransform;
};

using GltfIndexType = uint;
using GltfVertexType = float3;

struct Material {
	float4 color;
	float3 emissive;
	float metalness;
	float roughness;
};

constexpr auto DefaultMaterial = Material{
	.color = {1.0f, 1.0f, 1.0f, 1.0f},
	.emissive = {0.0f, 0.0f, 0.0f},
	.metalness = 0.0f,
	.roughness = 0.0f,
};

struct Mesh {
	Material material;
	stx::pvector<IndexType> indices;
	stx::pvector<VertexType> vertices;
};

using GltfData = std::unique_ptr<cgltf_data, decltype([](auto* p) {
	cgltf_free(p);
})>;

auto loadGltf(char const* _path) -> GltfData {

	auto const Options = cgltf_options{.type = cgltf_file_type_glb};

	auto* gltfPtr = static_cast<cgltf_data*>(nullptr);
	if (auto result = cgltf_parse_file(&Options, _path, &gltfPtr); result != cgltf_result_success)
		throw stx::runtime_error_fmt(R"(Failed to parse input mesh "{}": error code {})", _path, +result);
	cgltf_load_buffers(&Options, gltfPtr, nullptr);

	return GltfData(gltfPtr);

}

template<std::output_iterator<Material> T>
void getGltfMaterials(GltfData& _gltf, T _iter) {

	if (_gltf->materials_count == 0) {

		std::fprintf(stderr, "WARNING: Material data not present, using fallback\n");
		*_iter++ = DefaultMaterial;

	} else {

		for (auto i: stx::iota(0_zu, _gltf->materials_count)) {

			auto& material = _gltf->materials[i];
			auto& pbr = material.pbr_metallic_roughness;
			*_iter++ = Material{
				.color = float4{
					pbr.base_color_factor[0],
					pbr.base_color_factor[1],
					pbr.base_color_factor[2],
					pbr.base_color_factor[3],
				},
				.emissive = float3{
					material.emissive_factor[0],
					material.emissive_factor[1],
					material.emissive_factor[2],
				},
				.metalness = pbr.metallic_factor,
				.roughness = pbr.roughness_factor,
			};

		}

	}

}

auto getGltfNodeTransform(cgltf_node const& _node) -> float4x4 {

	auto translation = _node.has_translation?
		float4x4::translate(float3{_node.translation[0], _node.translation[1], _node.translation[2]}) :
		float4x4::identity();
	auto rotation = _node.has_rotation?
		float4x4::rotate(quat{_node.rotation[3], _node.rotation[0], _node.rotation[1], _node.rotation[2]}) :
		float4x4::identity();
	auto scale = _node.has_scale?
		float4x4::scale(float3{_node.scale[0], _node.scale[1], _node.scale[2]}) :
		float4x4::identity();

	auto transform = scale;
	transform = mul(transform, rotation);
	transform = mul(transform, translation);
	return transform;

}

int main(int argc, char const* argv[]) try {

	if (argc != 3)
		throw stx::runtime_error_fmt("Invalid number of arguments: found {}, expected 2", argc - 1);
	
	auto gltf = loadGltf(argv[1]);
	auto materials = stx::pvector<Material>();
	getGltfMaterials(gltf, std::back_inserter(materials));
	
	// Queue up the base nodes
	
	auto worknodes = stx::ivector<Worknode>();
	VERIFY(gltf->scenes_count == 1);
	auto& scene = gltf->scenes[0];
	for (auto i: stx::iota(0u, scene.nodes_count)) {
		
		auto node = scene.nodes[i];
		worknodes.emplace_back(Worknode{
			.node = node,
			.parentTransform = float4x4::identity() });
		
	}
	
	// Iterate over the node hierarchy
	
	auto meshes = stx::ivector<Mesh>();
	meshes.reserve(gltf->meshes_count);
	while (!worknodes.empty()) {
		
		auto worknode = worknodes.back();
		worknodes.pop_back();
		auto& node = *worknode.node;
		
		// Compute the transform
		
		auto transform = getGltfNodeTransform(node);
		transform = mul(transform, worknode.parentTransform);
		
		// Queue up all children nodes
		
		for (auto i: stx::iota(0u, node.children_count)) {
			
			auto child = node.children[i];
			worknodes.emplace_back(Worknode{
				.node = child,
				.parentTransform = transform });
			
		}
		
		// Process the node's mesh
		
		if (!node.mesh)
			continue;
		auto& nodeMesh = *node.mesh;
		VERIFY(nodeMesh.primitives_count == 1);
		auto& primitive = nodeMesh.primitives[0];
		auto& mesh = meshes.emplace_back();
		
		// Fetch material
		
		auto materialIdx = primitive.material? (primitive.material - gltf->materials) : 0;
		mesh.material = materials[materialIdx];
		
		// Fetch index data
		
		auto& indexAccessor = *VERIFY(primitive.indices);
		auto* indexBuffer = VERIFY(static_cast<char const*>(indexAccessor.buffer_view->buffer->data));
		indexBuffer += indexAccessor.buffer_view->offset;
		auto indexCount = indexAccessor.count;
		
		VERIFY(indexAccessor.component_type == cgltf_component_type_r_16u ||
			indexAccessor.component_type == cgltf_component_type_r_32u);
		VERIFY(indexAccessor.type == cgltf_type_scalar);
		if (indexAccessor.component_type == cgltf_component_type_r_16u) {
			
			auto* indexTypedBuffer = reinterpret_cast<uint16 const*>(indexBuffer);
			mesh.indices.insert(mesh.indices.end(), indexTypedBuffer, indexTypedBuffer + indexCount);
			
		} else {
			
			auto* indexTypedBuffer = reinterpret_cast<uint const*>(indexBuffer);
			mesh.indices.insert(mesh.indices.end(), indexTypedBuffer, indexTypedBuffer + indexCount);
			
		}
		
		// Fetch all vertex attributes
		
		for (auto attrIdx: stx::iota(0_zu, primitive.attributes_count)) {
			
			auto& accessor = *primitive.attributes[attrIdx].data;
			auto* buffer = VERIFY(static_cast<char const*>(accessor.buffer_view->buffer->data));
			buffer += accessor.buffer_view->offset;
			
			// Vertex positions
			
			if (std::strcmp(primitive.attributes[attrIdx].name, "POSITION") == 0) {
				
				VERIFY(accessor.component_type == cgltf_component_type_r_32f);
				VERIFY(accessor.type == cgltf_type_vec3);
				
				auto vertexCount = accessor.count;
				mesh.vertices.resize(vertexCount);
				static_assert(sizeof(GltfVertexType) == sizeof(VertexType));
				std::memcpy(mesh.vertices.data(), buffer, vertexCount * sizeof(GltfVertexType));
				continue;
				
			}
			
			// Vertex normals (ignored)
			
			if (std::strcmp(primitive.attributes[attrIdx].name, "NORMAL") == 0)
				continue;
			
			// Unknown attribute
			std::fprintf(stderr, "WARNING: Ignoring unknown attribute: %s\n", primitive.attributes[attrIdx].name);
			
		}
		
		ASSERT(mesh.indices.size());
		ASSERT(mesh.vertices.size());

		// Transform vertex attributes

		for (auto& v: mesh.vertices)
			v = float3(mul(float4(v, 1.0f), transform));
		
	}
	
	// Optimize mesh data
	
	for(auto& mesh: meshes) {
		
		// Prepare data for meshoptimizer
		
		auto streams = std::to_array<meshopt_Stream>({
			{mesh.vertices.data(), sizeof(GltfVertexType), sizeof(GltfVertexType)},
		});
		
		// meshoptimizer assumptions
		static_assert(sizeof(GltfVertexType) == sizeof(float) * 3);
		static_assert(sizeof(GltfIndexType) == sizeof(unsigned int));
		
		// Generate remap table
		
		auto remapTemp = stx::pvector<unsigned int>(mesh.vertices.size());
		auto uniqueVertexCount = ASSERT(meshopt_generateVertexRemapMulti(remapTemp.data(),
			mesh.indices.data(), mesh.indices.size(), mesh.vertices.size(),
			streams.data(), streams.size()));
		
		// Apply remap
		
		auto verticesRemapped = stx::pvector<GltfVertexType>(uniqueVertexCount);
		auto indicesRemapped = stx::pvector<GltfIndexType>(mesh.indices.size());
		
		meshopt_remapVertexBuffer(verticesRemapped.data(),
			mesh.vertices.data(), mesh.vertices.size(), sizeof(GltfVertexType),
			remapTemp.data());
		meshopt_remapIndexBuffer(indicesRemapped.data(),
			mesh.indices.data(), mesh.indices.size(), remapTemp.data());
		
		mesh.vertices = std::move(verticesRemapped);
		mesh.indices = std::move(indicesRemapped);
		
		ASSERT(mesh.vertices.size() == uniqueVertexCount);
		
		// Optimize for memory efficiency
		
		meshopt_optimizeVertexCache(mesh.indices.data(), mesh.indices.data(), mesh.indices.size(), mesh.vertices.size());
		meshopt_optimizeOverdraw(mesh.indices.data(), mesh.indices.data(), mesh.indices.size(),
			&mesh.vertices[0].x(), mesh.vertices.size(), sizeof(GltfVertexType),
			1.05f);
		
		remapTemp.resize(mesh.vertices.size());
		uniqueVertexCount = ASSERT(meshopt_optimizeVertexFetchRemap(remapTemp.data(),
			mesh.indices.data(), mesh.indices.size(), mesh.vertices.size()));
		
		verticesRemapped = stx::pvector<GltfVertexType>(uniqueVertexCount);
		indicesRemapped = stx::pvector<GltfIndexType>(mesh.indices.size());
		
		meshopt_remapVertexBuffer(verticesRemapped.data(),
			mesh.vertices.data(), mesh.vertices.size(), sizeof(GltfVertexType),
			remapTemp.data());
		meshopt_remapIndexBuffer(indicesRemapped.data(),
			mesh.indices.data(), mesh.indices.size(), remapTemp.data());
		
		mesh.vertices = std::move(verticesRemapped);
		mesh.indices = std::move(indicesRemapped);
		
		ASSERT(mesh.vertices.size() == uniqueVertexCount);
		
	}
	
	// Serialize model to msgpack
	
	auto const* outputPath = argv[2];
	auto out = mpack_writer_t();
	mpack_writer_init_filename(&out, outputPath);
	if (out.error != mpack_ok)
		throw stx::runtime_error_fmt(R"(Failed to open output file "{}" for writing: error code {})", outputPath, +out.error);
	
	mpack_write_uint(&out, ModelMagic);
	mpack_write_cstr(&out, "meshes");
	mpack_start_array(&out, meshes.size());
	for (auto& mesh: meshes) {

		mpack_start_map(&out, 3);

			mpack_write_cstr(&out, "material");
			mpack_start_map(&out, 4);
				mpack_write_cstr(&out, "color");
				mpack_start_array(&out, 4);
					mpack_write_float(&out, mesh.material.color.r());
					mpack_write_float(&out, mesh.material.color.g());
					mpack_write_float(&out, mesh.material.color.b());
					mpack_write_float(&out, mesh.material.color.a());
				mpack_finish_array(&out);
				mpack_write_cstr(&out, "emissive");
				mpack_start_array(&out, 3);
					mpack_write_float(&out, mesh.material.emissive.r());
					mpack_write_float(&out, mesh.material.emissive.g());
					mpack_write_float(&out, mesh.material.emissive.b());
				mpack_finish_array(&out);
				mpack_write_cstr(&out, "metalness");
				mpack_write_float(&out, mesh.material.metalness);
				mpack_write_cstr(&out, "roughness");
				mpack_write_float(&out, mesh.material.roughness);
			mpack_finish_map(&out);

			mpack_write_cstr(&out, "indices");
			mpack_write_bin(&out, reinterpret_cast<const char*>(mesh.indices.data()),
				mesh.indices.size() * sizeof(IndexType));

			mpack_write_cstr(&out, "vertices");
			mpack_write_bin(&out, reinterpret_cast<const char*>(mesh.vertices.data()),
				mesh.vertices.size() * sizeof(VertexType));
			
		mpack_finish_map(&out);

	}
	mpack_finish_array(&out);
	
	if (auto error = mpack_writer_destroy(&out); error != mpack_ok)
		throw stx::runtime_error_fmt(R"(Failed to write output file "{}": error code {})", outputPath, +error);
	
	return 0;
	
} catch (std::exception const& e) {
	
	printf("Runtime error: %s\n", e.what());
	return 1;
	
}

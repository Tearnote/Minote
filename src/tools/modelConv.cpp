#include <cassert>
#include <cstring>
#include "meshoptimizer.h"
#include "mpack/mpack.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#include "base/containers/vector.hpp"
#include "base/error.hpp"
#include "base/math.hpp"
#include "base/util.hpp"
#include "tools/modelSchema.hpp"
#include "tools/oct.hpp"

using namespace minote;
using namespace base;
using namespace base::literals;
using namespace tools;

struct Worknode {
	cgltf_node* node;
	mat4 parentTransform;
};

struct Material {
	vec4 color;
	vec3 emissive;
	f32 metalness;
	f32 roughness;
};

using RawIndexType = u32;
using RawVertexType = vec3;
using RawNormalType = vec3;

struct RawMesh {
	mat4 transform;
	u32 materialIdx;
	
	pvector<RawIndexType> indices;
	pvector<RawVertexType> vertices;
	pvector<RawNormalType> normals;
};

struct Meshlet {
	u32 indexOffset;
	u32 indexCount;
	u32 vertexOffset;
	
	vec3 boundingSphereCenter;
	f32 boundingSphereRadius;
};

struct Mesh {
	u32 materialIdx;
	
	pvector<Meshlet> meshlets;
	pvector<TriIndexType> triIndices;
	pvector<VertIndexType> vertIndices;
	pvector<VertexType> vertices;
	pvector<NormalType> normals;
};

int main(int argc, char const* argv[]) {
	
	if (argc != 3)
		throw runtime_error_fmt(R"(Invalid number of arguments: found {}, expected 2)", argc);
	
	// Load and parse input gltf
	
	auto const* inputPath = argv[1];
	auto options = cgltf_options{ .type = cgltf_file_type_glb };
	auto* gltf = static_cast<cgltf_data*>(nullptr);
	if (auto result = cgltf_parse_file(&options, inputPath, &gltf); result != cgltf_result_success)
		throw runtime_error_fmt(R"(Failed to parse input mesh "{}": error code {})", inputPath, result);
	defer { cgltf_free(gltf); };
	cgltf_load_buffers(&options, gltf, nullptr);
	
	// Fetch materials
	
	auto materials = ivector<Material>();
	materials.reserve(gltf->materials_count);
	for (auto i: iota(0_zu, gltf->materials_count)) {
		
		auto& material = gltf->materials[i];
		auto& pbr = material.pbr_metallic_roughness;
		materials.emplace_back(Material{
			.color = vec4{
				pbr.base_color_factor[0],
				pbr.base_color_factor[1],
				pbr.base_color_factor[2],
				pbr.base_color_factor[3] },
			.emissive = vec3{
				material.emissive_factor[0],
				material.emissive_factor[1],
				material.emissive_factor[2] },
			.metalness = pbr.metallic_factor,
			.roughness = pbr.roughness_factor });
		
	}
	if (gltf->materials_count == 0)
		fmt::print(stderr, "WARNING: Material data not present\n");
	
	// Queue up the base nodes
	
	auto worknodes = ivector<Worknode>();
	assert(gltf->scenes_count == 1);
	auto& scene = gltf->scenes[0];
	for (auto i: iota(0u, scene.nodes_count)) {
		
		auto node = scene.nodes[i];
		worknodes.emplace_back(Worknode{
			.node = node,
			.parentTransform = mat4::identity() });
		
	}
	
	// Iterate over the node hierarchy
	
	auto rawMeshes = ivector<RawMesh>();
	rawMeshes.reserve(gltf->meshes_count);
	while (!worknodes.empty()) {
		
		auto worknode = worknodes.back();
		worknodes.pop_back();
		auto& node = *worknode.node;
		
		// Compute the transform
		
		auto translation = node.has_translation?
			mat4::translate(vec3{node.translation[0], node.translation[1], node.translation[2]}) :
			mat4::identity();
		auto rotation = node.has_rotation?
			mat4::rotate(quat{node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]}) :
			mat4::identity();
		auto scale = node.has_scale?
			mat4::scale(vec3{node.scale[0], node.scale[1], node.scale[2]}) :
			mat4::identity();
		auto transform = worknode.parentTransform * translation * rotation * scale;
		
		// Queue up all children nodes
		
		for (auto i: iota(0u, node.children_count)) {
			
			auto child = node.children[i];
			worknodes.emplace_back(Worknode{
				.node = child,
				.parentTransform = transform });
			
		}
		
		// Process the node's mesh
		
		if (!node.mesh)
			continue;
		auto& nodeMesh = *node.mesh;
		assert(nodeMesh.primitives_count == 1);
		auto& primitive = nodeMesh.primitives[0];
		auto& mesh = rawMeshes.emplace_back();
		mesh.transform = transform;
		
		// Fetch material index
		
		mesh.materialIdx = primitive.material? (primitive.material - gltf->materials) : 0;
		
		// Fetch index data
		
		assert(primitive.indices);
		auto& indexAccessor = *primitive.indices;
		auto* indexBuffer = static_cast<char const*>(indexAccessor.buffer_view->buffer->data);
		assert(indexBuffer);
		indexBuffer += indexAccessor.buffer_view->offset;
		auto indexCount = indexAccessor.count;
		
		assert(indexAccessor.component_type == cgltf_component_type_r_16u ||
			indexAccessor.component_type == cgltf_component_type_r_32u);
		assert(indexAccessor.type == cgltf_type_scalar);
		if (indexAccessor.component_type == cgltf_component_type_r_16u) {
			
			auto* indexTypedBuffer = reinterpret_cast<u16 const*>(indexBuffer);
			mesh.indices.insert(mesh.indices.end(), indexTypedBuffer, indexTypedBuffer + indexCount);
			
		} else {
			
			auto* indexTypedBuffer = reinterpret_cast<u32 const*>(indexBuffer);
			mesh.indices.insert(mesh.indices.end(), indexTypedBuffer, indexTypedBuffer + indexCount);
			
		}
		
		// Fetch all vertex attributes
		
		for (auto attrIdx: iota(0_zu, primitive.attributes_count)) {
			
			auto& accessor = *primitive.attributes[attrIdx].data;
			auto* buffer = static_cast<char const*>(accessor.buffer_view->buffer->data);
			assert(buffer);
			buffer += accessor.buffer_view->offset;
			
			// Vertex positions
			
			if (std::strcmp(primitive.attributes[attrIdx].name, "POSITION") == 0) {
				
				assert(accessor.component_type == cgltf_component_type_r_32f);
				assert(accessor.type == cgltf_type_vec3);
				
				auto vertexCount = accessor.count;
				mesh.vertices.resize(vertexCount);
				std::memcpy(mesh.vertices.data(), buffer, vertexCount * sizeof(RawVertexType));
				continue;
				
			}
			
			// Vertex normals
			
			if (std::strcmp(primitive.attributes[attrIdx].name, "NORMAL") == 0) {
				
				assert(accessor.component_type == cgltf_component_type_r_32f);
				assert(accessor.type == cgltf_type_vec3);
				
				auto vertexCount = accessor.count;
				mesh.normals.resize(vertexCount);
				std::memcpy(mesh.normals.data(), buffer, vertexCount * sizeof(RawNormalType));
				continue;
				
			}
			
			// Unknown attribute
			fmt::print(stderr, "WARNING: Ignoring unknown attribute: {}\n", primitive.attributes[attrIdx].name);
			
		}
		
		assert(mesh.indices.size());
		assert(mesh.vertices.size());
		assert(mesh.normals.size());
		assert(mesh.vertices.size() == mesh.normals.size());
		
	}
	
	// Optimize mesh data
	
	for(auto& mesh: rawMeshes) {
		
		// Prepare data for meshoptimizer
		
		auto streams = to_array<meshopt_Stream>({
			{mesh.vertices.data(), sizeof(RawVertexType), sizeof(RawVertexType)},
			{mesh.normals.data(), sizeof(RawNormalType), sizeof(RawNormalType)},
		});
		
		// meshoptimizer assumptions
		static_assert(sizeof(RawVertexType) == sizeof(float) * 3);
		static_assert(sizeof(RawIndexType) == sizeof(unsigned int));
		
		// Generate remap table
		
		auto remapTemp = pvector<unsigned int>(mesh.vertices.size());
		auto uniqueVertexCount = meshopt_generateVertexRemapMulti(remapTemp.data(),
			mesh.indices.data(), mesh.indices.size(), mesh.vertices.size(),
			streams.data(), streams.size());
		
		assert(uniqueVertexCount);
		
		// Apply remap
		
		auto verticesRemapped = pvector<RawVertexType>(uniqueVertexCount);
		auto normalsRemapped = pvector<RawNormalType>(uniqueVertexCount);
		auto indicesRemapped = pvector<RawIndexType>(mesh.indices.size());
		
		meshopt_remapVertexBuffer(verticesRemapped.data(),
			mesh.vertices.data(), mesh.vertices.size(), sizeof(RawVertexType),
			remapTemp.data());
		meshopt_remapVertexBuffer(normalsRemapped.data(),
			mesh.normals.data(), mesh.normals.size(), sizeof(RawNormalType),
			remapTemp.data());
		meshopt_remapIndexBuffer(indicesRemapped.data(),
			mesh.indices.data(), mesh.indices.size(), remapTemp.data());
		
		mesh.vertices = std::move(verticesRemapped);
		mesh.normals = std::move(normalsRemapped);
		mesh.indices = std::move(indicesRemapped);
		
		assert(mesh.vertices.size() == uniqueVertexCount);
		assert(mesh.normals.size() == uniqueVertexCount);
		
		// Optimize for memory efficiency
		
		meshopt_optimizeVertexCache(mesh.indices.data(), mesh.indices.data(), mesh.indices.size(), mesh.vertices.size());
		meshopt_optimizeOverdraw(mesh.indices.data(), mesh.indices.data(), mesh.indices.size(),
			&mesh.vertices[0].x(), mesh.vertices.size(), sizeof(RawVertexType),
			1.05f);
		
		remapTemp.resize(mesh.vertices.size());
		uniqueVertexCount = meshopt_optimizeVertexFetchRemap(remapTemp.data(),
			mesh.indices.data(), mesh.indices.size(), mesh.vertices.size());
		assert(uniqueVertexCount);
		
		verticesRemapped = pvector<RawVertexType>(uniqueVertexCount);
		normalsRemapped = pvector<RawNormalType>(uniqueVertexCount);
		indicesRemapped = pvector<RawIndexType>(mesh.indices.size());
		
		meshopt_remapVertexBuffer(verticesRemapped.data(),
			mesh.vertices.data(), mesh.vertices.size(), sizeof(RawVertexType),
			remapTemp.data());
		meshopt_remapVertexBuffer(normalsRemapped.data(),
			mesh.normals.data(), mesh.normals.size(), sizeof(RawNormalType),
			remapTemp.data());
		meshopt_remapIndexBuffer(indicesRemapped.data(),
			mesh.indices.data(), mesh.indices.size(), remapTemp.data());
		
		mesh.vertices = std::move(verticesRemapped);
		mesh.normals = std::move(normalsRemapped);
		mesh.indices = std::move(indicesRemapped);
		
		assert(mesh.vertices.size() == uniqueVertexCount);
		assert(mesh.normals.size() == uniqueVertexCount);
		
	}
	
	// Convert meshes into meshlet representation
	
	auto meshes = ivector<Mesh>();
	
	for (auto& rawMesh: rawMeshes) {
		
		auto& mesh = meshes.emplace_back();
		mesh.materialIdx = rawMesh.materialIdx;
		
		// Pre-transform vertices
		
		mesh.vertices.reserve(rawMesh.vertices.size());
		for (auto v: rawMesh.vertices)
			mesh.vertices.push_back(vec3(rawMesh.transform * vec4(v, 1.0f)));
		
		// Convert normals to oct encoding
		
		mesh.normals.reserve(rawMesh.normals.size());
		for (auto n: rawMesh.normals)
			mesh.normals.push_back(octEncode(n));
		
		// Generate the meshlets
		
		// meshoptimizer assumptions
		static_assert(sizeof(TriIndexType) == sizeof(unsigned char));
		static_assert(sizeof(VertIndexType) == sizeof(unsigned int));
		
		auto maxMeshletCount = meshopt_buildMeshletsBound(rawMesh.indices.size(), MeshletMaxVerts, MeshletMaxTris);
		auto rawMeshlets = pvector<meshopt_Meshlet>(maxMeshletCount);
		auto meshletVertices = pvector<unsigned int>(maxMeshletCount * MeshletMaxVerts);
		auto meshletTriangles = pvector<unsigned char>(maxMeshletCount * MeshletMaxTris * 3);
		auto meshletCount = meshopt_buildMeshlets(rawMeshlets.data(), meshletVertices.data(), meshletTriangles.data(),
			rawMesh.indices.data(), rawMesh.indices.size(), &rawMesh.vertices[0].x(), rawMesh.vertices.size(), sizeof(RawVertexType),
			MeshletMaxVerts, MeshletMaxTris, MeshletConeWeight);
		rawMeshlets.resize(meshletCount);
		
		auto& lastMeshlet = rawMeshlets.back();
		meshletVertices.resize(lastMeshlet.vertex_offset + lastMeshlet.vertex_count);
		meshletTriangles.resize(lastMeshlet.triangle_offset + ((lastMeshlet.triangle_count * 3 + 3) & ~3));
		
		// Write meshlet data
		
		mesh.triIndices = std::move(meshletTriangles);
		mesh.vertIndices = std::move(meshletVertices);
		
		mesh.meshlets.reserve(meshletCount);
		for (auto& rawMeshlet: rawMeshlets) {
			
			auto& meshlet = mesh.meshlets.emplace_back();
			
			meshlet.indexOffset = rawMeshlet.triangle_offset;
			meshlet.indexCount = (rawMeshlet.triangle_count * 3 + 3) & ~3;
			meshlet.vertexOffset = rawMeshlet.vertex_offset;
			
			//TODO culling data
			meshlet.boundingSphereCenter = vec3(0.0f);
			meshlet.boundingSphereRadius = 0.0f;
			
		}
		
	}
	
	// Write to msgpack output
	
	auto const* outputPath = argv[2];
	auto model = mpack_writer_t();
	mpack_writer_init_filename(&model, outputPath);
	if (model.error != mpack_ok)
		throw runtime_error_fmt(R"(Failed to open output file "{}" for writing: error code {})", outputPath, model.error);
	
	mpack_write_u32(&model, ModelMagic);
	mpack_start_map(&model, 2);
		
		mpack_write_cstr(&model, "materials");
		mpack_start_array(&model, materials.size());
		for (auto& material: materials) {
			
			mpack_start_map(&model, 4);
				mpack_write_cstr(&model, "color");
				mpack_start_array(&model, 4);
					mpack_write_float(&model, material.color.r());
					mpack_write_float(&model, material.color.g());
					mpack_write_float(&model, material.color.b());
					mpack_write_float(&model, material.color.a());
				mpack_finish_array(&model);
				mpack_write_cstr(&model, "emissive");
				mpack_start_array(&model, 3);
					mpack_write_float(&model, material.emissive.r());
					mpack_write_float(&model, material.emissive.g());
					mpack_write_float(&model, material.emissive.b());
				mpack_finish_array(&model);
				mpack_write_cstr(&model, "metalness");
				mpack_write_float(&model, material.metalness);
				mpack_write_cstr(&model, "roughness");
				mpack_write_float(&model, material.roughness);
			mpack_finish_map(&model);
			
		}
		mpack_finish_array(&model);
		
		mpack_write_cstr(&model, "meshes");
		mpack_start_array(&model, meshes.size());
		for (auto& mesh: meshes) {
			
			mpack_start_map(&model, 6);
				
				mpack_write_cstr(&model, "materialIdx");
				mpack_write_u32(&model, mesh.materialIdx);
				
				mpack_write_cstr(&model, "meshlets");
				mpack_start_array(&model, mesh.meshlets.size());
				for (auto& meshlet: mesh.meshlets) {
					
					mpack_start_map(&model, 5);
						mpack_write_cstr(&model, "indexOffset");
						mpack_write_u32(&model, meshlet.indexOffset);
						mpack_write_cstr(&model, "indexCount");
						mpack_write_u32(&model, meshlet.indexCount);
						mpack_write_cstr(&model, "vertexOffset");
						mpack_write_u32(&model, meshlet.vertexOffset);
						mpack_write_cstr(&model, "boundingSphereCenter");
						mpack_start_array(&model, 3);
							mpack_write_float(&model, meshlet.boundingSphereCenter.x());
							mpack_write_float(&model, meshlet.boundingSphereCenter.y());
							mpack_write_float(&model, meshlet.boundingSphereCenter.z());
						mpack_finish_array(&model);
						mpack_write_cstr(&model, "boundingSphereRadius");
						mpack_write_float(&model, meshlet.boundingSphereRadius);
					mpack_finish_map(&model);
					
				}
				mpack_finish_array(&model);
				
				mpack_write_cstr(&model, "triIndices");
				mpack_write_bin(&model, reinterpret_cast<const char*>(mesh.triIndices.data()),
					mesh.triIndices.size() * sizeof(TriIndexType));
				
				mpack_write_cstr(&model, "vertIndices");
				mpack_write_bin(&model, reinterpret_cast<const char*>(mesh.vertIndices.data()),
					mesh.vertIndices.size() * sizeof(VertIndexType));
				
				mpack_write_cstr(&model, "vertices");
				mpack_write_bin(&model, reinterpret_cast<const char*>(mesh.vertices.data()),
					mesh.vertices.size() * sizeof(VertexType));
				
				mpack_write_cstr(&model, "normals");
				mpack_write_bin(&model, reinterpret_cast<const char*>(mesh.normals.data()),
					mesh.normals.size() * sizeof(NormalType));
				
			mpack_finish_map(&model);
			
		}
		mpack_finish_array(&model);
		
	mpack_finish_map(&model);
	
	if (auto error = mpack_writer_destroy(&model); error != mpack_ok)
		throw runtime_error_fmt(R"(Failed to write output file "{}": error code {})", outputPath, error);
	
	return 0;
	
}

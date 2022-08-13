#include <cstring>
#include <cmath>
#include "meshoptimizer.h"
#include "mpack/mpack.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#include "util/vector.hpp"
#include "util/verify.hpp"
#include "util/error.hpp"
#include "util/math.hpp"
#include "util/util.hpp"
#include "tools/modelSchema.hpp"
#include "tools/oct.hpp"

using namespace minote;

struct Worknode {
	cgltf_node* node;
	float4x4 parentTransform;
};

using GltfIndexType = uint;
using GltfVertexType = float3;
using GltfNormalType = float3;

struct GltfMesh {
	float4x4 transform;
	uint materialIdx;
	
	pvector<GltfIndexType> indices;
	pvector<GltfVertexType> vertices;
	pvector<GltfNormalType> normals;
};

struct Material {
	float4 color;
	float3 emissive;
	float metalness;
	float roughness;
};

struct Meshlet {
	uint materialIdx;
	
	uint indexOffset;
	uint indexCount;
	uint vertexOffset;
	
	float3 boundingSphereCenter;
	float boundingSphereRadius;
};

struct Model {
	pvector<Material> materials;
	pvector<Meshlet> meshlets;
	pvector<TriIndexType> triIndices;
	pvector<VertIndexType> vertIndices;
	pvector<VertexType> vertices;
	pvector<NormalType> normals;
};

int main(int argc, char const* argv[]) try {
	if (argc != 3)
		throw runtime_error_fmt("Invalid number of arguments: found {}, expected 2", argc - 1);
	
	auto model = Model();
	
	// Load and parse input gltf
	
	auto const* inputPath = argv[1];
	auto options = cgltf_options{ .type = cgltf_file_type_glb };
	auto* gltf = static_cast<cgltf_data*>(nullptr);
	
	if (auto result = cgltf_parse_file(&options, inputPath, &gltf); result != cgltf_result_success)
		throw runtime_error_fmt(R"(Failed to parse input mesh "{}": error code {})", inputPath, result);
	defer { cgltf_free(gltf); };
	cgltf_load_buffers(&options, gltf, nullptr);
	
	// Fetch materials
	
	model.materials.reserve(gltf->materials_count);
	for (auto i: iota(0_zu, gltf->materials_count)) {
		
		auto& material = gltf->materials[i];
		auto& pbr = material.pbr_metallic_roughness;
		model.materials.emplace_back(Material{
			.color = float4{
				pbr.base_color_factor[0],
				pbr.base_color_factor[1],
				pbr.base_color_factor[2],
				pbr.base_color_factor[3] },
			.emissive = float3{
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
	VERIFY(gltf->scenes_count == 1);
	auto& scene = gltf->scenes[0];
	for (auto i: iota(0u, scene.nodes_count)) {
		
		auto node = scene.nodes[i];
		worknodes.emplace_back(Worknode{
			.node = node,
			.parentTransform = float4x4::identity() });
		
	}
	
	// Iterate over the node hierarchy
	
	auto meshes = ivector<GltfMesh>();
	meshes.reserve(gltf->meshes_count);
	while (!worknodes.empty()) {
		
		auto worknode = worknodes.back();
		worknodes.pop_back();
		auto& node = *worknode.node;
		
		// Compute the transform
		
		auto translation = node.has_translation?
			float4x4::translate(float3{node.translation[0], node.translation[1], node.translation[2]}) :
			float4x4::identity();
		auto rotation = node.has_rotation?
			float4x4::rotate(quat{node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]}) :
			float4x4::identity();
		auto scale = node.has_scale?
			float4x4::scale(float3{node.scale[0], node.scale[1], node.scale[2]}) :
			float4x4::identity();
		auto transform = scale;
		transform = mul(transform, rotation);
		transform = mul(transform, translation);
		transform = mul(transform, worknode.parentTransform);
		
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
		VERIFY(nodeMesh.primitives_count == 1);
		auto& primitive = nodeMesh.primitives[0];
		auto& mesh = meshes.emplace_back();
		mesh.transform = transform;
		
		// Fetch material index
		
		mesh.materialIdx = primitive.material? (primitive.material - gltf->materials) : 0;
		
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
		
		for (auto attrIdx: iota(0_zu, primitive.attributes_count)) {
			
			auto& accessor = *primitive.attributes[attrIdx].data;
			auto* buffer = VERIFY(static_cast<char const*>(accessor.buffer_view->buffer->data));
			buffer += accessor.buffer_view->offset;
			
			// Vertex positions
			
			if (std::strcmp(primitive.attributes[attrIdx].name, "POSITION") == 0) {
				
				VERIFY(accessor.component_type == cgltf_component_type_r_32f);
				VERIFY(accessor.type == cgltf_type_vec3);
				
				auto vertexCount = accessor.count;
				mesh.vertices.resize(vertexCount);
				std::memcpy(mesh.vertices.data(), buffer, vertexCount * sizeof(GltfVertexType));
				continue;
				
			}
			
			// Vertex normals
			
			if (std::strcmp(primitive.attributes[attrIdx].name, "NORMAL") == 0) {
				
				VERIFY(accessor.component_type == cgltf_component_type_r_32f);
				VERIFY(accessor.type == cgltf_type_vec3);
				
				auto vertexCount = accessor.count;
				mesh.normals.resize(vertexCount);
				std::memcpy(mesh.normals.data(), buffer, vertexCount * sizeof(GltfNormalType));
				continue;
				
			}
			
			// Unknown attribute
			fmt::print(stderr, "WARNING: Ignoring unknown attribute: {}\n", primitive.attributes[attrIdx].name);
			
		}
		
		ASSERT(mesh.indices.size());
		ASSERT(mesh.vertices.size());
		ASSERT(mesh.normals.size());
		ASSERT(mesh.vertices.size() == mesh.normals.size());
		
	}
	
	// Optimize mesh data
	
	for(auto& mesh: meshes) {
		
		// Prepare data for meshoptimizer
		
		auto streams = to_array<meshopt_Stream>({
			{mesh.vertices.data(), sizeof(GltfVertexType), sizeof(GltfVertexType)},
			{mesh.normals.data(), sizeof(GltfNormalType), sizeof(GltfNormalType)},
		});
		
		// meshoptimizer assumptions
		static_assert(sizeof(GltfVertexType) == sizeof(float) * 3);
		static_assert(sizeof(GltfIndexType) == sizeof(unsigned int));
		
		// Generate remap table
		
		auto remapTemp = pvector<unsigned int>(mesh.vertices.size());
		auto uniqueVertexCount = ASSERT(meshopt_generateVertexRemapMulti(remapTemp.data(),
			mesh.indices.data(), mesh.indices.size(), mesh.vertices.size(),
			streams.data(), streams.size()));
		
		// Apply remap
		
		auto verticesRemapped = pvector<GltfVertexType>(uniqueVertexCount);
		auto normalsRemapped = pvector<GltfNormalType>(uniqueVertexCount);
		auto indicesRemapped = pvector<GltfIndexType>(mesh.indices.size());
		
		meshopt_remapVertexBuffer(verticesRemapped.data(),
			mesh.vertices.data(), mesh.vertices.size(), sizeof(GltfVertexType),
			remapTemp.data());
		meshopt_remapVertexBuffer(normalsRemapped.data(),
			mesh.normals.data(), mesh.normals.size(), sizeof(GltfNormalType),
			remapTemp.data());
		meshopt_remapIndexBuffer(indicesRemapped.data(),
			mesh.indices.data(), mesh.indices.size(), remapTemp.data());
		
		mesh.vertices = std::move(verticesRemapped);
		mesh.normals = std::move(normalsRemapped);
		mesh.indices = std::move(indicesRemapped);
		
		ASSERT(mesh.vertices.size() == uniqueVertexCount);
		ASSERT(mesh.normals.size() == uniqueVertexCount);
		
		// Optimize for memory efficiency
		
		meshopt_optimizeVertexCache(mesh.indices.data(), mesh.indices.data(), mesh.indices.size(), mesh.vertices.size());
		meshopt_optimizeOverdraw(mesh.indices.data(), mesh.indices.data(), mesh.indices.size(),
			&mesh.vertices[0].x(), mesh.vertices.size(), sizeof(GltfVertexType),
			1.05f);
		
		remapTemp.resize(mesh.vertices.size());
		uniqueVertexCount = ASSERT(meshopt_optimizeVertexFetchRemap(remapTemp.data(),
			mesh.indices.data(), mesh.indices.size(), mesh.vertices.size()));
		
		verticesRemapped = pvector<GltfVertexType>(uniqueVertexCount);
		normalsRemapped = pvector<GltfNormalType>(uniqueVertexCount);
		indicesRemapped = pvector<GltfIndexType>(mesh.indices.size());
		
		meshopt_remapVertexBuffer(verticesRemapped.data(),
			mesh.vertices.data(), mesh.vertices.size(), sizeof(GltfVertexType),
			remapTemp.data());
		meshopt_remapVertexBuffer(normalsRemapped.data(),
			mesh.normals.data(), mesh.normals.size(), sizeof(GltfNormalType),
			remapTemp.data());
		meshopt_remapIndexBuffer(indicesRemapped.data(),
			mesh.indices.data(), mesh.indices.size(), remapTemp.data());
		
		mesh.vertices = std::move(verticesRemapped);
		mesh.normals = std::move(normalsRemapped);
		mesh.indices = std::move(indicesRemapped);
		
		ASSERT(mesh.vertices.size() == uniqueVertexCount);
		ASSERT(mesh.normals.size() == uniqueVertexCount);
		
	}
	
	// Convert meshes into meshlet representation
	
	for (auto& mesh: meshes) {
		
		// Pre-transform vertices
		
		auto vertices = pvector<VertexType>();
		vertices.reserve(mesh.vertices.size());
		for (auto v: mesh.vertices)
			vertices.push_back(float3(mul(float4(v, 1.0f), mesh.transform)));
		
		// Pre-transform normals
		
		auto normTransform = transpose(inverse(mesh.transform));
		auto normals = pvector<GltfNormalType>();
		normals.reserve(mesh.normals.size());
		for (auto n: mesh.normals)
			normals.push_back(normalize(float3(mul(float4(n, 0.0f), normTransform))));
		
		// Convert normals to oct encoding
		
		auto octNormals = pvector<NormalType>();
		octNormals.reserve(mesh.normals.size());
		for (auto n: normals)
			octNormals.push_back(octEncode(n));
		
		// Generate the meshlets
		
		// meshoptimizer assumptions
		static_assert(sizeof(TriIndexType) == sizeof(unsigned char));
		static_assert(sizeof(VertIndexType) == sizeof(unsigned int));
		
		auto maxMeshletCount = meshopt_buildMeshletsBound(mesh.indices.size(), MeshletMaxVerts, MeshletMaxTris);
		auto rawMeshlets = pvector<meshopt_Meshlet>(maxMeshletCount);
		auto meshletVertices = pvector<unsigned int>(maxMeshletCount * MeshletMaxVerts);
		auto meshletTriangles = pvector<unsigned char>(maxMeshletCount * MeshletMaxTris * 3);
		auto meshletCount = meshopt_buildMeshlets(rawMeshlets.data(), meshletVertices.data(), meshletTriangles.data(),
			mesh.indices.data(), mesh.indices.size(), &vertices[0].x(), vertices.size(), sizeof(VertexType),
			MeshletMaxVerts, MeshletMaxTris, 0.0f);
		rawMeshlets.resize(meshletCount);
		
		auto& lastMeshlet = rawMeshlets.back();
		meshletVertices.resize(lastMeshlet.vertex_offset + lastMeshlet.vertex_count);
		meshletTriangles.resize(lastMeshlet.triangle_offset + ((lastMeshlet.triangle_count * 3 + 3) & ~3));
		
		// Generate meshlet bounds
		
		auto bounds = pvector<meshopt_Bounds>();
		bounds.reserve(meshletCount);
		for (auto& m: rawMeshlets) {
			
			bounds.emplace_back(meshopt_computeMeshletBounds(
				&meshletVertices[m.vertex_offset],
				&meshletTriangles[m.triangle_offset], m.triangle_count,
				&vertices[0].x(), vertices.size(), sizeof(VertexType)));
			
		}
		
		// Offset the vertex indices
		
		for (auto& idx: meshletVertices)
			idx += model.vertices.size();
		
		// Write meshlet descriptor
		
		model.meshlets.reserve(model.meshlets.size() + meshletCount);
		for (auto mIdx: iota(0_zu, rawMeshlets.size())) {
			
			auto& rawMeshlet = rawMeshlets[mIdx];
			auto& bound = bounds[mIdx];
			auto& meshlet = model.meshlets.emplace_back();
			
			meshlet.materialIdx = mesh.materialIdx;
			
			meshlet.indexOffset = rawMeshlet.triangle_offset + model.triIndices.size();
			meshlet.indexCount = (rawMeshlet.triangle_count * 3 + 3) & ~3;
			meshlet.vertexOffset = rawMeshlet.vertex_offset + model.vertIndices.size();
			
			meshlet.boundingSphereCenter = float3{bound.center[0], bound.center[1], bound.center[2]};
			meshlet.boundingSphereRadius = bound.radius;
			
		}
		
		// Append meshlet data
		
		model.triIndices.insert(model.triIndices.end(), meshletTriangles.begin(), meshletTriangles.end());
		model.vertIndices.insert(model.vertIndices.end(), meshletVertices.begin(), meshletVertices.end());
		model.vertices.insert(model.vertices.end(), vertices.begin(), vertices.end());
		model.normals.insert(model.normals.end(), octNormals.begin(), octNormals.end());
		
	}
	
	// Serialize model to msgpack
	
	auto const* outputPath = argv[2];
	auto out = mpack_writer_t();
	mpack_writer_init_filename(&out, outputPath);
	if (out.error != mpack_ok)
		throw runtime_error_fmt(R"(Failed to open output file "{}" for writing: error code {})", outputPath, out.error);
	
	mpack_write_uint(&out, ModelMagic);
	mpack_start_map(&out, 6);
		
		mpack_write_cstr(&out, "materials");
		mpack_start_array(&out, model.materials.size());
		for (auto& material: model.materials) {
			
			mpack_start_map(&out, 4);
				mpack_write_cstr(&out, "color");
				mpack_start_array(&out, 4);
					mpack_write_float(&out, material.color.r());
					mpack_write_float(&out, material.color.g());
					mpack_write_float(&out, material.color.b());
					mpack_write_float(&out, material.color.a());
				mpack_finish_array(&out);
				mpack_write_cstr(&out, "emissive");
				mpack_start_array(&out, 3);
					mpack_write_float(&out, material.emissive.r());
					mpack_write_float(&out, material.emissive.g());
					mpack_write_float(&out, material.emissive.b());
				mpack_finish_array(&out);
				mpack_write_cstr(&out, "metalness");
				mpack_write_float(&out, material.metalness);
				mpack_write_cstr(&out, "roughness");
				mpack_write_float(&out, material.roughness);
			mpack_finish_map(&out);
			
		}
		mpack_finish_array(&out);
		
		mpack_write_cstr(&out, "meshlets");
		mpack_start_array(&out, model.meshlets.size());
		for (auto& meshlet: model.meshlets) {
			
			mpack_start_map(&out, 8);
				
				mpack_write_cstr(&out, "materialIdx");
				mpack_write_uint(&out, meshlet.materialIdx);
				mpack_write_cstr(&out, "indexOffset");
				mpack_write_uint(&out, meshlet.indexOffset);
				mpack_write_cstr(&out, "indexCount");
				mpack_write_uint(&out, meshlet.indexCount);
				mpack_write_cstr(&out, "vertexOffset");
				mpack_write_uint(&out, meshlet.vertexOffset);
				mpack_write_cstr(&out, "boundingSphereCenter");
				mpack_start_array(&out, 3);
					mpack_write_float(&out, meshlet.boundingSphereCenter.x());
					mpack_write_float(&out, meshlet.boundingSphereCenter.y());
					mpack_write_float(&out, meshlet.boundingSphereCenter.z());
				mpack_finish_array(&out);
				mpack_write_cstr(&out, "boundingSphereRadius");
				mpack_write_float(&out, meshlet.boundingSphereRadius);
			mpack_finish_map(&out);
			
		}
		mpack_finish_array(&out);
		
		mpack_write_cstr(&out, "triIndices");
		mpack_write_bin(&out, reinterpret_cast<const char*>(model.triIndices.data()),
			model.triIndices.size() * sizeof(TriIndexType));
		
		mpack_write_cstr(&out, "vertIndices");
		mpack_write_bin(&out, reinterpret_cast<const char*>(model.vertIndices.data()),
			model.vertIndices.size() * sizeof(VertIndexType));
		
		mpack_write_cstr(&out, "vertices");
		mpack_write_bin(&out, reinterpret_cast<const char*>(model.vertices.data()),
			model.vertices.size() * sizeof(VertexType));
		
		mpack_write_cstr(&out, "normals");
		mpack_write_bin(&out, reinterpret_cast<const char*>(model.normals.data()),
			model.normals.size() * sizeof(NormalType));
		
	mpack_finish_map(&out);
	
	if (auto error = mpack_writer_destroy(&out); error != mpack_ok)
		throw runtime_error_fmt(R"(Failed to write output file "{}": error code {})", outputPath, error);
	
	return 0;
	
} catch (std::exception const& e) {
	
	printf("Runtime error: %s\n", e.what());
	return 1;
	
}

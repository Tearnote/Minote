#include <cassert>
#include <cstring>
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
	
	struct Material {
		vec4 color;
		vec3 emissive;
		f32 metalness;
		f32 roughness;
	};
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
	
	struct Worknode {
		cgltf_node* node;
		mat4 parentTransform;
	};
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
	
	struct Mesh {
		mat4 transform;
		u32 materialIdx;
		f32 radius;
		u32 indexCount;
		u32 vertexCount;
		pvector<IndexType> indices;
		pvector<VertexType> vertices;
		pvector<vec3> rawNormals;
		pvector<NormalType> normals;
	};
	auto meshes = ivector<Mesh>();
	meshes.reserve(gltf->meshes_count);
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
		auto& mesh = meshes.emplace_back();
		mesh.transform = transform;
		
		// Fetch material index
		
		mesh.materialIdx = primitive.material? (primitive.material - gltf->materials) : 0;
		
		// Fetch index data
		
		assert(primitive.indices);
		auto& indexAccessor = *primitive.indices;
		auto* indexBuffer = static_cast<char const*>(indexAccessor.buffer_view->buffer->data);
		assert(indexBuffer);
		indexBuffer += indexAccessor.buffer_view->offset;
		mesh.indexCount = indexAccessor.count;
		
		assert(indexAccessor.component_type == cgltf_component_type_r_16u ||
			indexAccessor.component_type == cgltf_component_type_r_32u);
		assert(indexAccessor.type == cgltf_type_scalar);
		if (indexAccessor.component_type == cgltf_component_type_r_16u) {
			
			auto* indexTypedBuffer = reinterpret_cast<u16 const*>(indexBuffer);
			mesh.indices.insert(mesh.indices.end(), indexTypedBuffer, indexTypedBuffer + indexAccessor.count);
			
		} else {
			
			auto* indexTypedBuffer = reinterpret_cast<u32 const*>(indexBuffer);
			mesh.indices.insert(mesh.indices.end(), indexTypedBuffer, indexTypedBuffer + indexAccessor.count);
			
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
				
				// Calculate the AABB and furthest point from the origin
				auto aabbMin = vec3{accessor.min[0], accessor.min[1], accessor.min[2]};
				auto aabbMax = vec3{accessor.max[0], accessor.max[1], accessor.max[2]};
				auto pfar = max(abs(aabbMin), abs(aabbMax));
				
				mesh.radius = length(pfar);
				mesh.vertexCount = accessor.count;
				mesh.vertices.resize(accessor.count);
				std::memcpy(mesh.vertices.data(), buffer, accessor.count * sizeof(VertexType));
				continue;
				
			}
			
			// Vertex normals
			
			if (std::strcmp(primitive.attributes[attrIdx].name, "NORMAL") == 0) {
				
				assert(accessor.component_type == cgltf_component_type_r_32f);
				assert(accessor.type == cgltf_type_vec3);
				
				mesh.rawNormals.resize(accessor.count);
				std::memcpy(mesh.rawNormals.data(), buffer, accessor.count * sizeof(vec3));
				continue;
				
			}
			
			// Unknown attribute
			fmt::print(stderr, "WARNING: Unknown attribute: {}\n", primitive.attributes[attrIdx].name);
			
		}
		
		assert(mesh.indexCount > 0_zu);
		assert(mesh.vertexCount > 0_zu);
		assert(mesh.indices.size());
		assert(mesh.vertices.size());
		assert(mesh.rawNormals.size());
		assert(mesh.radius > 0.0f);
		
	}
	
	// Convert to model schema
	
	for(auto& mesh: meshes) {
		
		mesh.normals.resize(mesh.vertexCount);
		for (auto i: iota(0_zu, mesh.vertexCount))
			mesh.normals[i] = octEncode(mesh.rawNormals[i]);
		
		assert(mesh.normals.size());
	
	}
	
	// Write to msgpack output
	
	auto const* outputPath = argv[2];
	auto model = mpack_writer_t();
	mpack_writer_init_filename(&model, outputPath);
	if (model.error != mpack_ok)
		throw runtime_error_fmt(R"(Failed to open output file "{}" for writing: error code {})", outputPath, model.error);
	
	mpack_start_map(&model, 3);
		mpack_write_cstr(&model, "format");
		mpack_write_u32(&model, ModelFormat);
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
			
			mpack_start_map(&model, 8);
				mpack_write_cstr(&model, "transform");
				mpack_start_array(&model, 16);
					for (auto y: iota(0, 4))
					for (auto x: iota(0, 4))
						mpack_write_float(&model, mesh.transform[x][y]);
				mpack_finish_array(&model);
				mpack_write_cstr(&model, "materialIdx");
				mpack_write_u32(&model, mesh.materialIdx);
				mpack_write_cstr(&model, "radius");
				mpack_write_float(&model, mesh.radius);
				mpack_write_cstr(&model, "indexCount");
				mpack_write_u32(&model, mesh.indexCount);
				mpack_write_cstr(&model, "vertexCount");
				mpack_write_u32(&model, mesh.vertexCount);
				mpack_write_cstr(&model, "indices");
				mpack_write_bin(&model, reinterpret_cast<const char*>(mesh.indices.data()), mesh.indexCount * sizeof(IndexType));
				mpack_write_cstr(&model, "vertices");
				mpack_write_bin(&model, reinterpret_cast<const char*>(mesh.vertices.data()), mesh.vertexCount * sizeof(VertexType));
				mpack_write_cstr(&model, "normals");
				mpack_write_bin(&model, reinterpret_cast<const char*>(mesh.normals.data()), mesh.vertexCount * sizeof(NormalType));
			mpack_finish_map(&model);
			
		}
		mpack_finish_array(&model);
	mpack_finish_map(&model);
	
	if (auto error = mpack_writer_destroy(&model); error != mpack_ok)
		throw runtime_error_fmt(R"(Failed to write output file "{}": error code {})", outputPath, error);
	
	return 0;
	
}

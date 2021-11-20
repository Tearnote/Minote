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
	
	// Choose mesh and primitive
	
	assert(gltf->meshes_count == 1);
	assert(gltf->meshes->primitives_count == 1);
	auto& primitive = *gltf->meshes->primitives;
	
	// Fetch material
	
	auto color = vec4(1.0f);
	auto pbrMetalness = 0.0f;
	auto pbrRoughness = 0.0f;
	if (auto* material = primitive.material; material && material->has_pbr_metallic_roughness) {
		
		auto& pbr = material->pbr_metallic_roughness;
		color = vec4{
			pbr.base_color_factor[0],
			pbr.base_color_factor[1],
			pbr.base_color_factor[2],
			pbr.base_color_factor[3] };
		pbrMetalness = pbr.metallic_factor;
		pbrRoughness = pbr.roughness_factor;
		
	} else {
		
		fmt::print(stderr, "WARNING: Material data not present in model\n");
		
	}
	
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
	auto rawIndices = pvector<u32>();
	if (indexAccessor.component_type == cgltf_component_type_r_16u) {
		
		auto* indexTypedBuffer = reinterpret_cast<u16 const*>(indexBuffer);
		rawIndices.insert(rawIndices.end(), indexTypedBuffer, indexTypedBuffer + indexAccessor.count);
		
	} else {
		
		auto* indexTypedBuffer = reinterpret_cast<u32 const*>(indexBuffer);
		rawIndices.insert(rawIndices.end(), indexTypedBuffer, indexTypedBuffer + indexAccessor.count);
		
	}
	
	// Fetch all vertex attributes
	
	auto radius = -1.0f;
	auto vertexCount = 0_zu;
	auto rawVertices = static_cast<vec3 const*>(nullptr);
	auto rawNormals = static_cast<vec3 const*>(nullptr);
	
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
			
			radius = length(pfar);
			vertexCount = accessor.count;
			rawVertices = reinterpret_cast<vec3 const*>(buffer);
			continue;
			
		}
		
		// Vertex normals
		
		if (std::strcmp(primitive.attributes[attrIdx].name, "NORMAL") == 0) {
			
			assert(accessor.component_type == cgltf_component_type_r_32f);
			assert(accessor.type == cgltf_type_vec3);
			
			rawNormals = reinterpret_cast<vec3 const*>(buffer);
			continue;
			
		}
		
		// Unknown attribute
		fmt::print(stderr, "WARNING: Unknown attribute: {}\n", primitive.attributes[attrIdx].name);
		
	}
	
	assert(indexCount > 0_zu);
	assert(vertexCount > 0_zu);
	assert(rawVertices);
	assert(rawNormals);
	assert(radius > 0.0f);
	
	// Convert to model schema
	
	auto& indices = rawIndices;
	auto* vertices = rawVertices;
	
	auto normals = pvector<NormalType>(vertexCount);
	for (auto i: iota(0_zu, vertexCount))
		normals[i] = octEncode(rawNormals[i]);
	
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
		mpack_start_array(&model, 1);
			mpack_start_map(&model, 3);
				mpack_write_cstr(&model, "color");
				mpack_start_array(&model, 4);
					mpack_write_float(&model, color.r());
					mpack_write_float(&model, color.g());
					mpack_write_float(&model, color.b());
					mpack_write_float(&model, color.a());
				mpack_finish_array(&model);
				mpack_write_cstr(&model, "metalness");
				mpack_write_float(&model, pbrMetalness);
				mpack_write_cstr(&model, "roughness");
				mpack_write_float(&model, pbrRoughness);
			mpack_finish_map(&model);
		mpack_finish_array(&model);
		mpack_write_cstr(&model, "meshes");
		mpack_start_array(&model, 1);
			mpack_start_map(&model, 7);
				mpack_write_cstr(&model, "materialIdx");
				mpack_write_u32(&model, 0);
				mpack_write_cstr(&model, "radius");
				mpack_write_float(&model, radius);
				mpack_write_cstr(&model, "indexCount");
				mpack_write_u32(&model, indexCount);
				mpack_write_cstr(&model, "vertexCount");
				mpack_write_u32(&model, vertexCount);
				mpack_write_cstr(&model, "indices");
				mpack_write_bin(&model, reinterpret_cast<const char*>(indices.data()), indexCount * sizeof(IndexType));
				mpack_write_cstr(&model, "vertices");
				mpack_write_bin(&model, reinterpret_cast<const char*>(vertices), vertexCount * sizeof(VertexType));
				mpack_write_cstr(&model, "normals");
				mpack_write_bin(&model, reinterpret_cast<const char*>(normals.data()), vertexCount * sizeof(NormalType));
			mpack_finish_map(&model);
		mpack_finish_array(&model);
	mpack_finish_map(&model);
	
	if (auto error = mpack_writer_destroy(&model); error != mpack_ok)
		throw runtime_error_fmt(R"(Failed to write output file "{}": error code {})", outputPath, error);
	
	return 0;
	
}

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
	auto rawColors = static_cast<u16vec4 const*>(nullptr);
	
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
		
		// Vertex colors
		
		if (std::strcmp(primitive.attributes[attrIdx].name, "COLOR_0") == 0) {
			
			assert(accessor.component_type == cgltf_component_type_r_16u);
			assert(accessor.type == cgltf_type_vec4);
			
			rawColors = reinterpret_cast<u16vec4 const*>(buffer);
			continue;
			
		}
		
		// Unknown attribute
		fmt::print(stderr, "WARNING: Unknown attribute: {}", primitive.attributes[attrIdx].name);
		
	}
	
	assert(indexCount > 0_zu);
	assert(vertexCount > 0_zu);
	assert(rawVertices);
	assert(rawNormals);
	assert(rawColors);
	assert(radius > 0.0f);
	
	// Convert to model schema
	
	auto& indices = rawIndices;
	auto* vertices = rawVertices;
	
	auto normals = pvector<NormalType>(vertexCount);
	for (auto i: iota(0_zu, vertexCount))
		normals[i] = octEncode(rawNormals[i]);
	
	auto colors = pvector<ColorType>(vertexCount);
	for (auto i: iota(0_zu, vertexCount)) {
		
		auto color = vec4{
			pow(f32(rawColors[i].r()) / f32(0xFFFF), 1.0f / 2.2f),
			pow(f32(rawColors[i].g()) / f32(0xFFFF), 1.0f / 2.2f),
			pow(f32(rawColors[i].b()) / f32(0xFFFF), 1.0f / 2.2f),
			f32(rawColors[i].a()) / f32(0xFFFF) };
		
		colors[i] = ColorType{
			u8(round(color.r() * f32(0xFF))),
			u8(round(color.g() * f32(0xFF))),
			u8(round(color.b() * f32(0xFF))),
			u8(round(color.a() * f32(0xFF))) };
		
	}
	
	// Write to msgpack output
	
	auto const* outputPath = argv[2];
	auto modelWriter = mpack_writer_t();
	mpack_writer_init_filename(&modelWriter, outputPath);
	if (modelWriter.error != mpack_ok)
		throw runtime_error_fmt(R"(Failed to open output file "{}" for writing: error code {})", outputPath, modelWriter.error);
	
	mpack_start_map(&modelWriter, ModelElements);
	mpack_write_cstr(&modelWriter, "format");
	mpack_write_u32(&modelWriter, ModelFormat);
	mpack_write_cstr(&modelWriter, "indexCount");
	mpack_write_u32(&modelWriter, indexCount);
	mpack_write_cstr(&modelWriter, "vertexCount");
	mpack_write_u32(&modelWriter, vertexCount);
	mpack_write_cstr(&modelWriter, "indices");
	mpack_write_bin(&modelWriter, reinterpret_cast<const char*>(indices.data()), indexCount * sizeof(IndexType));
	mpack_write_cstr(&modelWriter, "vertices");
	mpack_write_bin(&modelWriter, reinterpret_cast<const char*>(vertices), vertexCount * sizeof(VertexType));
	mpack_write_cstr(&modelWriter, "normals");
	mpack_write_bin(&modelWriter, reinterpret_cast<const char*>(normals.data()), vertexCount * sizeof(NormalType));
	mpack_write_cstr(&modelWriter, "colors");
	mpack_write_bin(&modelWriter, reinterpret_cast<const char*>(colors.data()), vertexCount * sizeof(ColorType));
	mpack_write_cstr(&modelWriter, "radius");
	mpack_write_float(&modelWriter, radius);
	mpack_finish_map(&modelWriter);
	
	if (auto error = mpack_writer_destroy(&modelWriter); error != mpack_ok)
		throw runtime_error_fmt(R"(Failed to write output file "{}": error code {})", outputPath, error);
	
	return 0;
	
}

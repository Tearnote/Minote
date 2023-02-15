#include "gfx/objects.hpp"

#include <array>
#include "vuk/Partials.hpp"
#include "util/vector.hpp"
#include "util/util.hpp"

namespace minote {

auto ObjectPool::create() -> ObjectID {
	
	if (m_deletedIDs.empty()) {
		metadata.emplace_back(Metadata::make_default());
		modelIDs.emplace_back();
		colors.emplace_back(float4{1.0f, 1.0f, 1.0f, 1.0f}); // Fully opaque
		transforms.emplace_back(Transform::make_default());
		prevTransforms.emplace_back(Transform::make_default());
		return size() - 1;
	} else {
		auto id = m_deletedIDs.back();
		m_deletedIDs.pop_back();
		metadata[id] = Metadata::make_default();
		colors[id] = float4{1.0f, 1.0f, 1.0f, 1.0f}; // Fully opaque
		transforms[id] = Transform::make_default();
		prevTransforms[id] = Transform::make_default();
		return id;
	}
	
}

void ObjectPool::destroy(ObjectID _id) {
	
	metadata[_id].exists = false;
	m_deletedIDs.push_back(_id);
	
}

auto ObjectPool::get(ObjectID _id) -> Proxy {
	
	return Proxy{
		.metadata = metadata[_id],
		.modelID = modelIDs[_id],
		.color = colors[_id],
		.transform = transforms[_id],
	};
	
}

auto ObjectPool::upload(vuk::Allocator& _allocator, ModelBuffer const& _models) -> ObjectBuffer {
	
	auto objectCount = sizeDrawable();
	
	// Prepare the space for data upload
	auto cpu_modelIndices = pvector<uint>();
	cpu_modelIndices.reserve(objectCount);
	auto cpu_transforms = pvector<ObjectBuffer::Transform>();
	cpu_transforms.reserve(objectCount);
	auto cpu_prevTransforms = pvector<ObjectBuffer::Transform>();
	cpu_prevTransforms.reserve(objectCount);
	auto cpu_colors = pvector<float4>();
	cpu_colors.reserve(objectCount);
	
	// Queue up all valid objects
	auto meshCount = 0u;
	auto triangleCount = 0u;
	for (auto idx: iota(ObjectID(0), size())) {
		auto& meta = metadata[idx];
		if (!meta.exists || !meta.visible)
			continue;
		
		auto modelID = modelIDs[idx];
		auto modelIdx = _models.cpu_modelIndices.at(modelID);
		meshCount += _models.cpu_models[modelIdx].meshCount;
		
		auto& model = _models.cpu_models[modelIdx];
		for (auto i: iota(0u, model.meshCount))
			triangleCount += _models.cpu_meshes[model.meshOffset + i].indexCount / 3u;
		
		cpu_modelIndices.emplace_back(modelIdx);
		cpu_transforms.emplace_back(encodeTransform(transforms[idx]));
		cpu_prevTransforms.emplace_back(encodeTransform(prevTransforms[idx]));
		cpu_colors.emplace_back(colors[idx]);
	}
	
	// Upload to GPU
	return ObjectBuffer {
		.modelIndices = vuk::create_buffer_cross_device(_allocator,
			vuk::MemoryUsage::eCPUtoGPU,
			span(cpu_modelIndices)).second,
		.colors = vuk::create_buffer_cross_device(_allocator,
			vuk::MemoryUsage::eCPUtoGPU,
			span(cpu_colors)).second,
		.transforms = vuk::create_buffer_cross_device(_allocator,
			vuk::MemoryUsage::eCPUtoGPU,
			span(cpu_transforms)).second,
		.prevTransforms = vuk::create_buffer_cross_device(_allocator,
			vuk::MemoryUsage::eCPUtoGPU,
			span(cpu_prevTransforms)).second,
		.objectCount = objectCount,
		.meshCount = meshCount,
		.triangleCount = triangleCount,
	};
	
}

void ObjectPool::copyTransforms() {
	
	prevTransforms = transforms;
	
}

auto ObjectPool::encodeTransform(ObjectPool::Transform _in) -> ObjectBuffer::Transform {
	
	auto rotationMat = float3x3::rotate(_in.rotation);
	
	rotationMat[0] *= _in.scale.x();
	rotationMat[1] *= _in.scale.y();
	rotationMat[2] *= _in.scale.z();
	
	return std::to_array({
		float4(rotationMat[0], _in.position.x()),
		float4(rotationMat[1], _in.position.y()),
		float4(rotationMat[2], _in.position.z()),
	});
	
}

auto ObjectPool::sizeDrawable() -> uint {
	
	auto result = 0u;
	for (auto& meta: metadata)
		result += meta.exists && meta.visible? 1 : 0;
	return result;
	
}

}

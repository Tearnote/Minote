#include "gfx/objects.hpp"

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
	
	auto modelCount = sizeDrawable();
	
	// Prepare the space for data upload
	auto cpu_transforms = pvector<ObjectBuffer::Transform>();
	cpu_transforms.reserve(modelCount);
	auto cpu_prevTransforms = pvector<ObjectBuffer::Transform>();
	cpu_prevTransforms.reserve(modelCount);
	auto cpu_colors = pvector<float4>();
	cpu_colors.reserve(modelCount);
	
	// Queue up all valid objects
	for (auto idx: iota(ObjectID(0), size())) {
		auto& meta = metadata[idx];
		if (!meta.exists || !meta.visible)
			continue;
		
		cpu_transforms.emplace_back(encodeTransform(transforms[idx]));
		cpu_prevTransforms.emplace_back(encodeTransform(prevTransforms[idx]));
		cpu_colors.emplace_back(colors[idx]);
	}
	
	// Upload to GPU
	return ObjectBuffer {
		.colors = vuk::create_buffer_cross_device(_allocator,
			vuk::MemoryUsage::eCPUtoGPU,
			span(cpu_colors)).second,
		.transforms = vuk::create_buffer_cross_device(_allocator,
			vuk::MemoryUsage::eCPUtoGPU,
			span(cpu_transforms)).second,
		.prevTransforms = vuk::create_buffer_cross_device(_allocator,
			vuk::MemoryUsage::eCPUtoGPU,
			span(cpu_prevTransforms)).second,
	};
	
}

void ObjectPool::copyTransforms() {
	
	prevTransforms = transforms;
	
}

auto ObjectPool::encodeTransform(ObjectPool::Transform _in) -> ObjectBuffer::Transform {
	
	auto rw = _in.rotation.w();
	auto rx = _in.rotation.x();
	auto ry = _in.rotation.y();
	auto rz = _in.rotation.z();
	
	auto rotationMat = float3x3{
		1.0f - 2.0f * (ry * ry + rz * rz),        2.0f * (rx * ry - rw * rz),        2.0f * (rx * rz + rw * ry),
		       2.0f * (rx * ry + rw * rz), 1.0f - 2.0f * (rx * rx + rz * rz),        2.0f * (ry * rz - rw * rx),
		       2.0f * (rx * rz - rw * ry),        2.0f * (ry * rz + rw * rx), 1.0f - 2.0f * (rx * rx + ry * ry),
	};
	
	rotationMat[0] *= _in.scale.x();
	rotationMat[1] *= _in.scale.y();
	rotationMat[2] *= _in.scale.z();
	
	return to_array({
		float4(rotationMat[0], _in.position.x()),
		float4(rotationMat[1], _in.position.y()),
		float4(rotationMat[2], _in.position.z()),
	});
	
}

auto ObjectPool::sizeDrawable() -> usize {
	
	auto result = 0u;
	for (auto& meta: metadata)
		result += meta.exists && meta.visible? 1 : 0;
	return result;
	
}

}

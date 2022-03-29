#include "gfx/effects/bvh.hpp"

namespace minote::gfx {

using namespace base;

void BVH::compile(vuk::PerThreadContext& _ptc) {
	
	auto debugAABBPci = vuk::PipelineBaseCreateInfo();
	debugAABBPci.add_spirv(std::vector<u32>{
#include "spv/bvh/debugAABB.vert.spv"
	}, "bvh/debugAABB.vert");
	debugAABBPci.add_spirv(std::vector<u32>{
#include "spv/bvh/debugAABB.frag.spv"
	}, "bvh/debugAABB.frag");
	_ptc.ctx.create_named_pipeline("bvh/debugAABB", debugAABBPci);
	
}

void BVH::debugDrawAABBs(Frame& _frame, Texture2D _target, InstanceList _instances) {
	
	auto aabbs = Buffer<AABB>::make(_frame.permPool, "AABBs",
		vuk::BufferUsageFlagBits::eStorageBuffer,
		_frame.models.cpu_meshletAABBs);
	aabbs.attach(_frame.rg, vuk::eHostWrite, vuk::eNone);
	
	_frame.rg.add_pass({
		.name = nameAppend(_target.name, "bvh/debugAABB"),
		.resources = {
			aabbs.resource(vuk::eVertexRead),
			_instances.instances.resource(vuk::eVertexRead),
			_instances.transforms.resource(vuk::eVertexRead),
			_target.resource(vuk::eColorWrite) },
		.execute = [_target, _instances, aabbs, &_frame](vuk::CommandBuffer& cmd) {
			
			cmd.set_viewport(0, vuk::Rect2D::framebuffer());
			cmd.set_scissor(0, vuk::Rect2D::framebuffer());
			cmd.set_color_blend(_target.name, vuk::BlendPreset::eOff);
			cmd.set_primitive_topology(vuk::PrimitiveTopology::eLineList);
			cmd.set_rasterization(vuk::PipelineRasterizationStateCreateInfo{
				.lineWidth = 1.0f });
			
			cmd.bind_uniform_buffer(0, 0, _frame.world)
			   .bind_storage_buffer(0, 1, aabbs)
			   .bind_storage_buffer(0, 2, _instances.instances)
			   .bind_storage_buffer(0, 3, _instances.transforms)
			   .bind_graphics_pipeline("bvh/debugAABB");
			
			cmd.draw(12 * 2, _instances.size(), 0, 0);
			
		}});
	
}

}

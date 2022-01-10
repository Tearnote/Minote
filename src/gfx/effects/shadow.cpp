#include "gfx/effects/shadow.hpp"

#include "vuk/CommandBuffer.hpp"
#include "base/util.hpp"
#include "base/math.hpp"
#include "gfx/samplers.hpp"
#include "gfx/util.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;

void Shadow::compile(vuk::PerThreadContext& _ptc) {
	
	auto genBufferPci = vuk::PipelineBaseCreateInfo();
	genBufferPci.add_spirv(std::vector<u32>{
#include "spv/shadow/genBuffer.vert.spv"
	}, "shadow/genBuffer.vert");
	genBufferPci.add_spirv(std::vector<u32>{
#include "spv/shadow/genBuffer.frag.spv"
	}, "shadow/genBuffer.frag");
	_ptc.ctx.create_named_pipeline("shadow/genBuffer", genBufferPci);
	
	auto genShadowPci = vuk::ComputePipelineBaseCreateInfo();
	genShadowPci.add_spirv(std::vector<u32>{
#include "spv/shadow/genShadow.comp.spv"
	}, "shadow/genShadow.comp");
	_ptc.ctx.create_named_pipeline("shadow/genShadow", genShadowPci);
	
}

void Shadow::genBuffer(Frame& _frame, Texture2D _shadowbuf, InstanceList _instances) {
	
	auto depth = Texture2D::make(_frame.permPool, nameAppend(_shadowbuf.name, "depth"),
		_shadowbuf.size(), vuk::Format::eD32Sfloat,
		vuk::ImageUsageFlagBits::eDepthStencilAttachment);
	depth.attach(_frame.rg, vuk::eClear, vuk::eNone, vuk::ClearDepthStencil(0.0f, 0));
	
	_frame.rg.add_pass({
		.name = nameAppend(_shadowbuf.name, "shadow/genBuffer"),
		.resources = {
			_instances.commands.resource(vuk::eIndirectRead),
			_instances.instances.resource(vuk::eVertexRead),
			_instances.transforms.resource(vuk::eVertexRead),
			_shadowbuf.resource(vuk::eColorWrite),
			depth.resource(vuk::eDepthStencilRW) },
		.execute = [&_frame, _shadowbuf, _instances](vuk::CommandBuffer& cmd) {
			
			auto view = look(_frame.cpu_world.cameraPos, _frame.cpu_world.sunDirection, {0.0f, 0.0f, -1.0f});
			auto projection = orthographic(64_m, 64_m, -256_m, 256_m);
			auto viewProjection = projection * view;
			
			cmd.set_viewport(0, vuk::Rect2D::framebuffer());
			cmd.set_scissor(0, vuk::Rect2D::framebuffer());
			cmd.set_color_blend(_shadowbuf.name, vuk::BlendPreset::eOff);
			cmd.set_rasterization(vuk::PipelineRasterizationStateCreateInfo{
				.cullMode = vuk::CullModeFlagBits::eFront });
			cmd.set_depth_stencil(vuk::PipelineDepthStencilStateCreateInfo{
				.depthTestEnable = true,
				.depthWriteEnable = true,
				.depthCompareOp = vuk::CompareOp::eGreater });
			
			cmd.bind_index_buffer(_frame.models.indices, vuk::IndexType::eUint32)
			   .bind_storage_buffer(0, 0, _frame.models.vertices)
			   .bind_storage_buffer(0, 1, _frame.models.meshes)
			   .bind_storage_buffer(0, 2, _instances.instances)
			   .bind_storage_buffer(0, 3, _instances.transforms)
			   .bind_graphics_pipeline("shadow/genBuffer");
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eVertex, 0, viewProjection);
			
			cmd.draw_indexed_indirect(_instances.commands.length(), _instances.commands);
			
		}});
	
}

void Shadow::genShadow(Frame& _frame, Texture2D _shadowbuf, Texture2D _shadowOut,
	QuadBuffer& _quadbuf, InstanceList _instances) {
	
	_frame.rg.add_pass({
		
		.name = nameAppend(_shadowOut.name, "shadow/genShadow"),
		.resources = {
			_instances.instances.resource(vuk::eComputeRead),
			_instances.transforms.resource(vuk::eComputeRead),
			_quadbuf.visbuf.resource(vuk::eComputeSampled),
			_quadbuf.offset.resource(vuk::eComputeSampled),
			_quadbuf.depth.resource(vuk::eComputeSampled),
			_shadowbuf.resource(vuk::eComputeSampled),
			_shadowOut.resource(vuk::eComputeWrite) },
		.execute = [&_frame, _shadowbuf, _shadowOut, _quadbuf, _instances](vuk::CommandBuffer& cmd) {
			
			auto view = look(_frame.cpu_world.cameraPos, _frame.cpu_world.sunDirection, {0.0f, 0.0f, -1.0f});
			auto projection = orthographic(64_m, 64_m, -256_m, 256_m);
			auto viewProjection = projection * view;
			
			cmd.bind_uniform_buffer(0, 0, _frame.world)
			   .bind_storage_buffer(0, 1, _frame.models.meshes)
			   .bind_storage_buffer(0, 2, _instances.instances)
			   .bind_storage_buffer(0, 3, _instances.transforms)
			   .bind_storage_buffer(0, 4, _frame.models.indices)
			   .bind_storage_buffer(0, 5, _frame.models.vertices)
			   .bind_sampled_image(0, 6, _quadbuf.visbuf, NearestClamp)
			   .bind_sampled_image(0, 7, _quadbuf.offset, NearestClamp)
			   .bind_sampled_image(0, 8, _quadbuf.depth, NearestClamp)
			   .bind_sampled_image(0, 9, _quadbuf.normal, NearestClamp)
			   .bind_sampled_image(0, 10, _shadowbuf, NearestClamp)
			   .bind_storage_image(0, 11, _shadowOut)
			   .bind_compute_pipeline("shadow/genShadow");
			
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, viewProjection);
			
			cmd.specialize_constants(0, u32Fromu16(_shadowbuf.size()));
			cmd.specialize_constants(1, u32Fromu16(_shadowOut.size()));
			
			cmd.dispatch_invocations(_shadowOut.size().x(), _shadowOut.size().y());
			
		}});
	
}

}

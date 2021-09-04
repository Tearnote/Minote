#include "gfx/effects/tonemap.hpp"

#include "vuk/CommandBuffer.hpp"
#include "base/types.hpp"
#include "gfx/util.hpp"

namespace minote::gfx {

using namespace base;

void Tonemap::compile(vuk::PerThreadContext& _ptc) {
	
	auto tonemapPci = vuk::PipelineBaseCreateInfo();
	tonemapPci.add_spirv(std::vector<u32>{
#include "spv/tonemap.vert.spv"
	}, "blit.vert");
	tonemapPci.add_spirv(std::vector<u32>{
#include "spv/tonemap.frag.spv"
	}, "blit.frag");
	_ptc.ctx.create_named_pipeline("tonemap", tonemapPci);
	
}

void Tonemap::apply(vuk::RenderGraph& _rg, vuk::Name _source, vuk::Name _target, uvec2 _targetSize) {
	
	_rg.add_pass({
		.name = "Tonemapping",
		.resources = {
			vuk::Resource(_source, vuk::Resource::Type::eImage, vuk::eFragmentSampled),
			vuk::Resource(_target, vuk::Resource::Type::eImage, vuk::eColorWrite) },
		.execute = [_source, _targetSize](vuk::CommandBuffer& cmd) {
			
			cmd.set_viewport(0, vuk::Rect2D{ .extent = vukExtent(_targetSize) })
			   .set_scissor(0, vuk::Rect2D{ .extent = vukExtent(_targetSize) })
			   .bind_sampled_image(0, 0, _source, {})
			   .bind_graphics_pipeline("tonemap");
			cmd.draw(3, 1, 0, 0);
			
		}});
	
}

}

#include "gfx/modules/tonemap.hpp"

#include "vuk/CommandBuffer.hpp"
#include "base/types.hpp"
#include "gfx/base.hpp"

namespace minote::gfx {

using namespace base;

Tonemap::Tonemap(vuk::PerThreadContext& _ptc) {
	
	if (!pipelinesCreated) {
		
		auto tonemapPci = vuk::PipelineBaseCreateInfo();
		tonemapPci.add_spirv(std::vector<u32>{
#include "spv/tonemap.vert.spv"
		}, "blit.vert");
		tonemapPci.add_spirv(std::vector<u32>{
#include "spv/tonemap.frag.spv"
		}, "blit.frag");
		_ptc.ctx.create_named_pipeline("tonemap", tonemapPci);
		
		pipelinesCreated = true;
		
	}
	
}

auto Tonemap::apply(vuk::Name _source, vuk::Name _target, uvec2 _targetSize) -> vuk::RenderGraph {
	
	auto rg = vuk::RenderGraph();
	
	rg.add_pass({
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
	
	return rg;
	
}

}

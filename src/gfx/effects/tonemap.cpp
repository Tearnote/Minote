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

void Tonemap::apply(vuk::RenderGraph& _rg, Texture2D _source, Texture2D _target) {
	
	_rg.add_pass({
		.name = nameAppend(_source.name, "tonemapping"),
		.resources = {
			_source.resource(vuk::eFragmentSampled),
			_target.resource(vuk::eColorWrite) },
		.execute = [_source, _target](vuk::CommandBuffer& cmd) {
			
			cmdSetViewportScissor(cmd, _target.size());
			cmd.bind_sampled_image(0, 0, _source, {})
			   .bind_graphics_pipeline("tonemap");
			cmd.draw(3, 1, 0, 0);
			
		}});
	
}

}

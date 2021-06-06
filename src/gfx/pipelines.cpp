#include "gfx/pipelines.hpp"

#include "base/types.hpp"

namespace minote::gfx {

using namespace base;

void createPipelines(vuk::Context& ctx) {
	auto tonemapPci = vuk::PipelineBaseCreateInfo();
	tonemapPci.add_spirv(std::vector<u32>{
#include "spv/tonemap.vert.spv"
	}, "blit.vert");
	tonemapPci.add_spirv(std::vector<u32>{
#include "spv/tonemap.frag.spv"
	}, "blit.frag");
	ctx.create_named_pipeline("tonemap", tonemapPci);

}

}

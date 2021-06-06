#include "gfx/pipelines.hpp"

#include "base/types.hpp"

namespace minote::gfx {

using namespace base;

void createPipelines(vuk::Context& ctx) {
	auto zPrepassPci = vuk::PipelineBaseCreateInfo();
	zPrepassPci.add_spirv(std::vector<u32>{
#include "spv/zprepass.vert.spv"
	}, "zprepass.vert");
	zPrepassPci.add_spirv(std::vector<u32>{
#include "spv/zprepass.frag.spv"
	}, "zprepass.frag");
	zPrepassPci.rasterization_state.cullMode = vuk::CullModeFlagBits::eBack;
	zPrepassPci.depth_stencil_state.depthCompareOp = vuk::CompareOp::eGreater;
	ctx.create_named_pipeline("z_prepass", zPrepassPci);

	auto objectPci = vuk::PipelineBaseCreateInfo();
	objectPci.add_spirv(std::vector<u32>{
#include "spv/object.vert.spv"
	}, "object.vert");
	objectPci.add_spirv(std::vector<u32>{
#include "spv/object.frag.spv"
	}, "object.frag");
	objectPci.rasterization_state.cullMode = vuk::CullModeFlagBits::eBack;
	objectPci.depth_stencil_state.depthWriteEnable = false;
	objectPci.depth_stencil_state.depthCompareOp = vuk::CompareOp::eEqual;
	ctx.create_named_pipeline("object", objectPci);

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

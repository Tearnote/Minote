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

	auto skyPci = vuk::PipelineBaseCreateInfo();
	skyPci.add_spirv(std::vector<u32>{
#include "spv/sky.vert.spv"
	}, "sky.vert");
	skyPci.add_spirv(std::vector<u32>{
#include "spv/sky.frag.spv"
	}, "sky.frag");
	skyPci.rasterization_state.cullMode = vuk::CullModeFlagBits::eFront;
	skyPci.depth_stencil_state.depthWriteEnable = false;
	skyPci.depth_stencil_state.depthCompareOp = vuk::CompareOp::eLessOrEqual;
	ctx.create_named_pipeline("sky", skyPci);

	auto tonemapPci = vuk::PipelineBaseCreateInfo();
	tonemapPci.add_spirv(std::vector<u32>{
#include "spv/tonemap.vert.spv"
	}, "blit.vert");
	tonemapPci.add_spirv(std::vector<u32>{
#include "spv/tonemap.frag.spv"
	}, "blit.frag");
	ctx.create_named_pipeline("tonemap", tonemapPci);

	auto cullPci = vuk::ComputePipelineCreateInfo();
	cullPci.add_spirv(std::vector<u32>{
#include "spv/cull.comp.spv"
	}, "cull.comp");
	ctx.create_named_pipeline("cull", cullPci);

	auto cubemapPci = vuk::ComputePipelineCreateInfo();
	cubemapPci.add_spirv(std::vector<u32>{
#include "spv/cubemap.comp.spv"
	}, "cubemap.comp");
	ctx.create_named_pipeline("cubemap", cubemapPci);

	auto cubemipPci = vuk::ComputePipelineCreateInfo();
	cubemipPci.add_spirv(std::vector<u32>{
#include "spv/cubemip.comp.spv"
	}, "cubemip.comp");
	cubemipPci.set_variable_count_binding(0, 0, 16);
	cubemipPci.set_binding_flags(0, 0, vuk::DescriptorBindingFlagBits::ePartiallyBound);
	ctx.create_named_pipeline("cubemip", cubemipPci);
}

}

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
	objectPci.depth_stencil_state.depthCompareOp = vuk::CompareOp::eEqual;
	ctx.create_named_pipeline("object", objectPci);

	auto skyPci = vuk::PipelineBaseCreateInfo();
	skyPci.add_spirv(std::vector<u32>{
#include "spv/sky.vert.spv"
	}, "sky.vert");
	skyPci.add_spirv(std::vector<u32>{
#include "spv/sky.frag.spv"
	}, "sky.frag");
	skyPci.depth_stencil_state.depthWriteEnable = false;
	ctx.create_named_pipeline("sky", skyPci);

	auto tonemapPci = vuk::PipelineBaseCreateInfo();
	tonemapPci.add_spirv(std::vector<u32>{
#include "spv/tonemap.vert.spv"
	}, "blit.vert");
	tonemapPci.add_spirv(std::vector<u32>{
#include "spv/tonemap.frag.spv"
	}, "blit.frag");
	ctx.create_named_pipeline("tonemap", tonemapPci);

	auto bloomThresholdPci = vuk::PipelineBaseCreateInfo();
	bloomThresholdPci.add_spirv(std::vector<u32>{
#include "spv/bloomThreshold.vert.spv"
	}, "bloomThreshold.vert");
	bloomThresholdPci.add_spirv(std::vector<u32>{
#include "spv/bloomThreshold.frag.spv"
	}, "bloomThreshold.frag");
	ctx.create_named_pipeline("bloom_threshold", bloomThresholdPci);

	auto bloomBlurDownPci = vuk::PipelineBaseCreateInfo();
	bloomBlurDownPci.add_spirv(std::vector<u32>{
#include "spv/bloomBlur.vert.spv"
	}, "bloomBlur.vert");
	bloomBlurDownPci.add_spirv(std::vector<u32>{
#include "spv/bloomBlur.frag.spv"
	}, "bloomBlur.frag");
	ctx.create_named_pipeline("bloom_blur_down", bloomBlurDownPci);

	auto bloomBlurUpPci = vuk::PipelineBaseCreateInfo();
	bloomBlurUpPci.add_spirv(std::vector<u32>{
#include "spv/bloomBlur.vert.spv"
	}, "bloomBlur.vert");
	bloomBlurUpPci.add_spirv(std::vector<u32>{
#include "spv/bloomBlur.frag.spv"
	}, "bloomBlur.frag");
	bloomBlurUpPci.set_blend(vuk::BlendPreset::eAlphaBlend);
	// Turn into additive
	bloomBlurUpPci.color_blend_attachments[0].srcColorBlendFactor = vuk::BlendFactor::eOne;
	bloomBlurUpPci.color_blend_attachments[0].dstColorBlendFactor = vuk::BlendFactor::eOne;
	bloomBlurUpPci.color_blend_attachments[0].dstAlphaBlendFactor = vuk::BlendFactor::eOne;
	ctx.create_named_pipeline("bloom_blur_up", bloomBlurUpPci);

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
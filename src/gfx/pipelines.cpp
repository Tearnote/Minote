#include "gfx/pipelines.hpp"

#include "base/types.hpp"

namespace minote::gfx {

using namespace base;

void createPipelines(vuk::Context& ctx) {
	auto objectPci = vuk::PipelineBaseCreateInfo();
	objectPci.add_spirv(std::vector<u32>{
#include "spv/object.vert.spv"
	}, "object.vert");
	objectPci.add_spirv(std::vector<u32>{
#include "spv/object.frag.spv"
	}, "object.frag");
	objectPci.rasterization_state.cullMode = vuk::CullModeFlagBits::eBack;
	ctx.create_named_pipeline("object", objectPci);

	auto cubemapPci = vuk::PipelineBaseCreateInfo();
	cubemapPci.add_spirv(std::vector<u32>{
#include "spv/cubemap.vert.spv"
	}, "cubemap.vert");
	cubemapPci.add_spirv(std::vector<u32>{
#include "spv/cubemap.frag.spv"
	}, "cubemap.frag");
	cubemapPci.depth_stencil_state.depthWriteEnable = false;
	ctx.create_named_pipeline("cubemap", cubemapPci);

	auto swapchainBlitPci = vuk::PipelineBaseCreateInfo();
	swapchainBlitPci.add_spirv(std::vector<u32>{
#include "spv/swapchainBlit.vert.spv"
	}, "blit.vert");
	swapchainBlitPci.add_spirv(std::vector<u32>{
#include "spv/swapchainBlit.frag.spv"
	}, "blit.frag");
	ctx.create_named_pipeline("swapchain_blit", swapchainBlitPci);

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
}

}

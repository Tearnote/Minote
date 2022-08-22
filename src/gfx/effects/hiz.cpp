#include "gfx/effects/hiz.hpp"

#include "vuk/partials/SPD.hpp"
#include "vuk/CommandBuffer.hpp"
#include "vuk/RenderGraph.hpp"
#include "gfx/samplers.hpp"
#include "gfx/shader.hpp"
#include "gfx/util.hpp"
#include "sys/vulkan.hpp"
#include "util/array.hpp"
#include "util/util.hpp"

namespace minote {

GET_SHADER(hiz_blit_cs);
void HiZ::compile() {
	
	if (m_compiled) return;
	auto& ctx = *s_vulkan->context;
	
	auto blitPci = vuk::PipelineBaseCreateInfo();
	ADD_SHADER(blitPci, hiz_blit_cs, "hiz/blit.cs.hlsl");
	ctx.create_named_pipeline("hiz/blit", blitPci);
	
	m_compiled = true;
	
}

HiZ::HiZ(Texture2D<float> _depth) {
	
	compile();
	
	auto rg = std::make_shared<vuk::RenderGraph>("hiz");
	rg->attach_in("depth", _depth);
	rg->attach_and_clear_image("hiz", vuk::ImageAttachment{
		.format = HiZFormat,
		.sample_count = vuk::Samples::e1,
		.layer_count = 1,
	}, vuk::ClearColor(1.0f, 1.0f, 1.0f, 1.0f));
	rg->inference_rule("hiz",
		[](vuk::InferenceContext const& ctx, vuk::ImageAttachment& ia) {
			auto& d = ctx.get_image_attachment("depth");
			auto size = max(
				nextPOT(d.extent.extent.width),
				nextPOT(d.extent.extent.height));
			ia.extent = vuk::Dimension3D::absolute(size, size);
			ia.level_count = mipmapCount(size);
		});
	
	rg->add_pass(vuk::Pass{
		.name = "hiz/blit",
		.resources = {
			"depth"_image >> vuk::eComputeSampled,
			"hiz"_image >> vuk::eComputeWrite >> "hiz/mip0",
		},
		.execute = [](vuk::CommandBuffer& cmd) {
			
			cmd.bind_compute_pipeline("hiz/blit")
			   .bind_image(0, 0, "depth").bind_sampler(0, 0, NearestClamp)
			   .bind_image(0, 1, "hiz");
			
			auto depth = *cmd.get_resource_image_attachment("depth");
			auto depthSize = depth.extent.extent;
			cmd.specialize_constants(0, depthSize.width);
			cmd.specialize_constants(1, depthSize.height);
			auto hiz = *cmd.get_resource_image_attachment("hiz");
			cmd.specialize_constants(2, hiz.extent.extent.width);
			
			cmd.dispatch_invocations(divRoundUp(depthSize.width, 2), divRoundUp(depthSize.height, 2));
			
		},
	});
	
	hiz = vuk::generate_mips_spd(*s_vulkan->context, vuk::Future(rg, "hiz/mip0"), vuk::ReductionType::Min);
	// hiz = vuk::Future(rg, "hiz/mip0");
	
}

}

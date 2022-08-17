#include "gfx/effects/hiz.hpp"

#include "vuk/CommandBuffer.hpp"
#include "vuk/RenderGraph.hpp"
#include "gfx/samplers.hpp"
#include "gfx/shader.hpp"
#include "gfx/util.hpp"
#include "sys/vulkan.hpp"
#include "util/array.hpp"
#include "util/util.hpp"

namespace minote {

void HiZ::compile() {
	
	if (m_compiled) return;
	auto& ctx = *s_vulkan->context;
	
	//
	
	m_compiled = true;
	
}

auto HiZ::create(Texture2D<float> _depth) -> Texture2D<float> {
	
	compile();
	
	auto rg = std::make_shared<vuk::RenderGraph>("hiz");
	rg->attach_in("depth", _depth);
	rg->attach_and_clear_image("hiz", vuk::ImageAttachment{
		.format = vuk::Format::eR32Sfloat,
		.sample_count = vuk::Samples::e1,
		.layer_count = 1,
	}, vuk::ClearColor(1.0f, 1.0f, 1.0f, 1.0f));
	rg->inference_rule("hiz",
		[](vuk::InferenceContext const& ctx, vuk::ImageAttachment& ia) {
			auto& d = ctx.get_image_attachment("depth");
			ia.extent.sizing = d.extent.sizing;
			uint size = max(
				nextPOT(d.extent.extent.width),
				nextPOT(d.extent.extent.height));
			size = max(size, 128u); // Can't be smaller than a single compute tile
			ia.extent.extent.width = size;
			ia.extent.extent.height = size;
			ia.layer_count = mipmapCount(size);
		});
	
	rg->add_pass({
		.name = "hiz/create",
		.resources = {
			"depth"_image >> vuk::eComputeSampled,
			"hiz"_image >> vuk::eComputeWrite >> "hiz/final",
		},
		.execute = [](vuk::CommandBuffer& cmd) {
			
			auto hiz = *cmd.get_resource_image_attachment("hiz");
			auto mipCount = hiz.level_count;
			auto mipsGenerated = 0u;
			
			auto hizMips = array<vuk::ImageAttachment, 13>();
			hizMips.fill(hiz);
			for (uint i: iota(0u, mipCount)) {
				hizMips[i].base_level = i;
				hizMips[i].level_count = 1;
			}
			
			// Initial pass
			cmd.bind_compute_pipeline("hiz/first")
			   .bind_image(0, 0, "depth").bind_sampler(0, 0, NearestClamp)
			   .bind_image(0, 1, hizMips[min(0u, mipCount - 1)])
			   .bind_image(0, 2, hizMips[min(1u, mipCount - 1)])
			   .bind_image(0, 3, hizMips[min(2u, mipCount - 1)])
			   .bind_image(0, 4, hizMips[min(3u, mipCount - 1)])
			   .bind_image(0, 5, hizMips[min(4u, mipCount - 1)])
			   .bind_image(0, 6, hizMips[min(5u, mipCount - 1)]);
			
			auto depth = *cmd.get_resource_image_attachment("depth");
			auto depthSize = depth.extent.extent;
			cmd.specialize_constants(0, depthSize.width);
			cmd.specialize_constants(1, depthSize.height);
			cmd.specialize_constants(2, hiz.extent.extent.width);
			cmd.specialize_constants(3, hiz.extent.extent.height);
			cmd.specialize_constants(4, min(mipCount, 6u));
			
			cmd.dispatch_invocations(1, divRoundUp(depthSize.width, 32 * 4), divRoundUp(depthSize.height, 32 * 4));
			mipsGenerated += 6;
			
			if (mipsGenerated >= mipCount)
				return;
			// We need to read from a mip to continue generating lower mips
			cmd.image_barrier("hiz", vuk::eComputeRW, vuk::eComputeSampled, mipsGenerated - 1, 1);
			
			// Followup pass
			cmd.bind_compute_pipeline("hiz/mips")
			   .bind_image(0, 0, hizMips[min(mipsGenerated - 1, mipCount - 1)]).bind_sampler(0, 0, MinClamp)
			   .bind_image(0, 1, hizMips[min(mipsGenerated + 0, mipCount - 1)])
			   .bind_image(0, 2, hizMips[min(mipsGenerated + 1, mipCount - 1)])
			   .bind_image(0, 3, hizMips[min(mipsGenerated + 2, mipCount - 1)])
			   .bind_image(0, 4, hizMips[min(mipsGenerated + 3, mipCount - 1)])
			   .bind_image(0, 5, hizMips[min(mipsGenerated + 4, mipCount - 1)])
			   .bind_image(0, 6, hizMips[min(mipsGenerated + 5, mipCount - 1)])
			   .bind_image(0, 7, hizMips[min(mipsGenerated + 6, mipCount - 1)]);
			
			auto inputMipSize = hiz.extent.extent.width >> (mipsGenerated - 1u);
			cmd.specialize_constants(0, inputMipSize);
			cmd.specialize_constants(1, min(mipCount - mipsGenerated, 7u));
			
			cmd.dispatch_invocations(depthSize.width / 4, depthSize.height / 4);
			
			// Converge the transitioned mip to the same state as the rest
			cmd.image_barrier("hiz", vuk::eComputeSampled, vuk::eComputeRW, mipsGenerated - 1, 1);
			
		},
	});
	
	return vuk::Future(rg, "hiz/final");
	
}

}
